#pragma once
#include <vector>
#include "Node.h"
using namespace std;

class Network {
public:
	vector<Node> nodeList;
	vector<int> innovation;
	double learningRate;
	Node *input;
	Node *output;
	double fitness;
	int networkId;
	int species;
	
	Network();
	void printNetwork();
	double* process(double input[]);
	double backProp(double input[], double desired[]);
	double trainset(double* input, int lim);
	int getInnovation(int pos);
	void addInnovation(int num);
	bool containsInnovation(int num);
	void removeInnovation(int num);

	void mutateConnection(int from, int to, int innovation);
	int numConnection();
	void resetWeight();

	Node* getNode(int i);
	Node* createNode(int send);
	int getNextNodeId();
	int mutateNode(int from, int to, int innovationA, int innovationB);
	bool checkCircleMaster(Node* n, int goal);
	bool checkCircle(Node* n, int goal, int preCheck[]);
};

Network clone(Network* n);