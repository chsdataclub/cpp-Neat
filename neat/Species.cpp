#include "stdafx.h"
#include "Species.h"
#include <algorithm>
#include<ctime> //for time function
#include<cstdlib>// for srand function.
using namespace std;

vector<pair<int, int>>* Species::innovationDict;

Species::Species(int id, vector<Network*> networks, double mutate)
{
	this->id = id;
	this->mutate = mutate;
	this->network = networks;

	updateStereotype();
}

void Species::addCI(int a)
{
	for (int i = 0; i < commonInnovation.size(); i++) {
		if (commonInnovation[i] == a) {
			return;
		}
	}

	commonInnovation.push_back(a);
}

void Species::removeCI(int a)
{
	for (int i = 0; i < commonInnovation.size(); i++) {
		if (commonInnovation[i] == a) {
			commonInnovation.erase(commonInnovation.begin() + i);
		}
	}
}

int& Species::getInovOcc(int i)
{
	if (i >= connectionInnovation.size()) {
		connectionInnovation.reserve(connectionInnovation.capacity() + i - connectionInnovation.size() + 1);
	}
	while (i >= connectionInnovation.size()) {
		connectionInnovation.push_back(0);
	}
	return (connectionInnovation)[i];
}

int& Species::incrementInov(int i)
{
	int& ans = getInovOcc(i);
	(ans)++;

	if (double((ans) / network.size()) >= .5) {
		addCI(i);
	}

	return ans;
}

int& Species::reduceInov(int i)
{
	int& ans = getInovOcc(i);
	(ans)--;

	if (ans == 0 || double((ans) / network.size()) < .5) {
		removeCI(i);
	}

	return ans;
}

void Species::checkCI()
{
	for (int i = 0; i < commonInnovation.size(); i++) {
		removeCI(commonInnovation[i]);
	}

	for (int i = 0; i < connectionInnovation.size(); i++) {
		if (double(connectionInnovation[i] / network.size()) >= .5) {
			addCI(i);
		}
	}
}

pair<int, int> Species::getInnovationRef(int num)
{
	return (*innovationDict)[num];
}

int Species::createNewInnovation(int a, int b)
{
	//dictControl.Lock() TODO: fix the multithread
	pair<int, int> c = { a, b};
	(*innovationDict).push_back(c); //TODO: simplify
	//defer dictControl.Unlock()

	return innovationDict->size() - 1;
}

void Species::sortInnovation()
{
	for (int i = 0; i < network.size(); i++) {
		sort(network[i]->innovation.begin(), network[i]->innovation.end());
	}
}

Network& Species::getNetworkAt(int a)
{
	return *network[a];
}

void Species::removeNetwork(int id)
{
	for (int i = 0; i < network.size(); i++) {
		if (network[i]->networkId == id) {
			vector<int> inn = network[i]->innovation;
			network.erase(network.begin() + i);

			for (int a = 0; a < inn.size(); a++) {
				reduceInov(inn[a]);
			}

			//checkCI();
		}
	}
}

Network& Species::getNetwork(int id)
{
	for (int i = 0; i < network.size(); i++) {
		if (network[i]->networkId == id) {
			return *network[i];
		}
	}
}

void Species::addNetwork(Network& n)
{
	n.species = id;
	network.push_back(&n);

	for (int i = 0; i < n.innovation.size(); i++) {
		incrementInov(n.innovation[i]);
	}

	checkCI();
}

void Species::updateStereotype()
{
	int numNodes = 0;

	connectionInnovation.clear();
	commonInnovation.clear();

	for (int i = 0; i < network.size(); i++) {
		numNodes += network[i]->nodeList.size();
		for (int a = 0; a < network[i]->innovation.size(); a++) {
			incrementInov(network[i]->innovation[a]);
		}
	}
}

void Species::mutateNetwork(Network& network)
{
	srand(time(NULL));
	int nodeRange = network.nodeList.size();

	//finds or adds innovation numbers and returns the innovation
	auto addConnectionInnovation = [&](int numFrom, int numTo) {
		//checks to see if preexisting innovation
		int maxPos = innovationDict->size();
		for (int i = 0; i < maxPos; i++) {
			pair<int, int> pos = getInnovationRef(i);
			if (pos.second == numTo && pos.first == numFrom) {
				incrementInov(i);

				return i;
			}
		}

		//checks to see if needs to grow
		int num = createNewInnovation(numFrom, numTo);

		incrementInov(num);

		return num;
	};

	//mutates a node
	auto nodeMutate = [&]() {
		int  firstNode = -1;
		int secondNode = -1;
		bool ans = false;

		//picks a node
		while (!ans) {
			firstNode = int(rand()%nodeRange);

			if (network.getNode(firstNode).send.size() > 0) {
				ans = true;
			}
		}

		//picks a random connection from firstNode and gets the id
		secondNode = network.getNode(firstNode).send[int(rand()%network.getNode(firstNode).send.size())].nodeTo->id; //int(r.Int63n(int64(nodeRange)));

		network.mutateNode(firstNode, secondNode, addConnectionInnovation(firstNode, network.getNextNodeId()), addConnectionInnovation(network.getNextNodeId(), secondNode));
	};

	int test = rand() % 100;
	//randomly picks if node or connection mutate
	if (test <= mutate*100) {
		nodeMutate();
	}
	else {
		int firstNode;
		int secondNode;
		//ans := true
		int attempts = 0; //attempts at finding nodes

							   //find 2 unconnected nodes
							   //for ans && attempts <= 10 {
		while (attempts <= 10) {
			srand(time(NULL));
			int t = rand();
			int a = rand();
			firstNode = t%nodeRange;
			secondNode = a%nodeRange;

			if (firstNode == secondNode || isOutput(network.getNode(firstNode)) || isInput(network.getNode(secondNode))) {
				continue;
			}

			if (network.getNode(firstNode).connectsTo(secondNode) || network.getNode(secondNode).connectsTo(firstNode) || network.checkCircleMaster(network.getNode(firstNode), secondNode)) {
				attempts++;
				continue;
			}
		}

		//muateNode if attempts exceeded else add the connection
		if (attempts > 10) {
			nodeMutate();
		}
		else {
			network.mutateConnection(firstNode, secondNode, addConnectionInnovation(firstNode, secondNode));
		}
	}
}

//TODO: see if returning a copy screws stuff up with the references
void Species::mateNetwork(vector<int>& nB, vector<int>& nA, int nodeNum, int nodeNumA, Network& ans)
{
	int in = network[0]->input.size() - 1;
	int out = network[0]->output.size();
	ans = Network(in, out, -1, id, network[0]->learningRate, false);

	int numNode = -1 * (in + out + 1); //subtract input and output nodes because those are already created
	if (nodeNum > nodeNumA) {
		numNode += nodeNum;
	}
	else {
		numNode += nodeNumA;
	}
	//create the nodes
	for (int i = 0; i < numNode; i++) { //this should be ok
		ans.createNode(100);
	}

	//add nA innovation
	for (int i = 0; i < nA.size(); i++) {
		pair<int, int> ina = getInnovationRef(nA[i]);
		ans.mutateConnection(ina.first, ina.second, nA[i]);
	}

	//add unique nB innovation
	for (int i = 0; i < nB.size(); i++) {
		pair<int, int> inb = getInnovationRef(nB[i]);
		int firstNode = inb.first;
		int secondNode = inb.second;

		//checks to make sure their is no conflict in possible innovations
		if (!ans.containsInnovation(nB[i]) && !(ans.getNode(firstNode).connectsTo(secondNode) || ans.getNode(secondNode).connectsTo(firstNode)) && !ans.checkCircleMaster(ans.getNode(firstNode), secondNode)) {
			pair<int, int> ina = getInnovationRef(nB[i]);
			ans.mutateConnection(ina.first, ina.second, nB[i]);
		}
	}
}

//TODO: multithread
void Species::trainNetworks(vector<pair<vector<double>,vector<double>>>& trainingSet)
{
	for (int i = 0; i < network.size(); i++) {
		network[i]->trainset(trainingSet, 10000); //I have capped the number of interations intentionaly to control training time
	}
}

void Species::mateSpecies()
{
	adjustFitness();

	//sorts by adjusted fitness
	vector<Network*> sortedNetwork;
	sortedNetwork.reserve((network.size() * 85 / 100));
	double lastValue = 1000.0;
	double sumFitness = 0.0;
	for (int i = 0; i < sortedNetwork.capacity(); i++) {
		double localMax = 0.0;
		int localIndex = 0;
		for (int a = 0; a < network.size(); a++) {
			if (getNetworkAt(a).adjustedFitness > localMax && getNetworkAt(a).adjustedFitness <= lastValue) {
				bool good = true;
				for (int b = i - 1; b >= 0; b--) {
					if (getNetworkAt(a).networkId == sortedNetwork[b]->networkId) {
						good = false;
						break;
					}

					if (sortedNetwork[b]->adjustedFitness != getNetworkAt(a).adjustedFitness) {
						break;
					}
				}

				if (good) {
					localMax = network[a]->adjustedFitness;
					localIndex = a;
				}
			}
		}

		sortedNetwork.push_back(&getNetworkAt(localIndex));
		sumFitness += sortedNetwork[i]->adjustedFitness;
		lastValue = sortedNetwork[i]->adjustedFitness;
	}

	int count = 0;
	//mates networks
	for (int i = 0; i < sortedNetwork.size(); i++) {
		int numKids = int(sortedNetwork[i]->adjustedFitness / sumFitness * network.size());
		int numMade = numKids;
		for (int a = 1; count < network.size() && a + i < sortedNetwork.size(); a++) {
			int id = network[count]->networkId;
			mateNetwork(sortedNetwork[i]->innovation, sortedNetwork[i + a]->innovation, sortedNetwork[i]->nodeList.size(), sortedNetwork[i + a]->nodeList.size(), *network[count]);
			network[count]->networkId = id;
			count++;
			numMade--;
		}
	}

	//mutates for remainder of spots available
	for (int i = 0; count < network.size(); i++) {
		int id = network[count]->networkId;
		clone(sortedNetwork[i], *network[count]);
		mutateNetwork(*network[count]);
		network[count]->networkId = id;
		count++;

		if (i == sortedNetwork.size() - 1) {
			i--; //this can lead to mutating the same network as last time (stacking mutations) but i don't think it is a big deal
		}
	}

	updateStereotype();
}

void Species::adjustFitness()
{
	for (int i = 0; i < network.size(); i++) {
		network[i]->adjustedFitness = network[i]->fitness / network.size();
	}
}

int Species::avgNode()
{
	if (network.size() == 0) {
		return 0;
	}
int sum = 0;
	for (int i = 0; i < network.size(); i++) {
		sum += network[i]->nodeList.size();
	}

	return sum / network.size();
}
