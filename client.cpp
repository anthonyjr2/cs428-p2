#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

int packetCounter = 0;

class node{
	public:
		int nodeid, ctrlPort, dataPort;
		string hostName;
	private:
}

typedef struct{
	uint8_t sourceNodeID;
	uint8_t destNodeID;
	uint8_t packetID;
	uint8_t TTL;
}controlPacketHeader;

typedef struct{
	int [2] pathToTravel;
}controlPacketPayload;

typedef struct{
	uint8_t sourceNodeID;
	uint8_t destNodeID;
	uint8_t packetID;
	uint8_t TTL
}dataPacketHeader;

typedef struct{
	int [100] pathTraveled;
}dataPacketPayload;

int main(int argc, char *argv[]){
	vector<node>nodeList;
	
	if(argc < 2){
		cout<<"usage : "<<argv[0]<<" config.txt"<<endl;
		exit(1);
	}
	

	if(argv[2] == "generate-packet"){
		if(generatePacket(atoi(argv[3]),atoi(argv[4]),nodeList) == -1){
			cout <<"Error sending packet"<<endl;
			exit(1);
		}
	}else if(argv[2] == "create-link"){
		if(createLink(atoi(argv[3]),atoi(argv[4])) == -1){
			cout << "Error creating link" << endl;
			exit(1);
		}
	}else if(argv[2] == "remove-Link"){
		if(removeLink(atoi(argv[3]),atoi(argv[4]))==-1){
			cout << "Error removing link" << endl;
			exit(1);
		}
	}else{
		nodeList = initialize(argv[2]);
		
	}

}

int generatePacket(int sender,int reciever,nodeList){
	string host = "";
	//step 1 create control packet
	controlPacketHeader newControlHeader;
	controlPacketPayload newControlPayLoad;
	//step 2 find the info for the sender packet
	for(int i = 0; i < nodeList.size();i++){
		if(nodeList[i].nodeid == sender){
			//we have found our man
			node temp = nodeList[i];
			newControlHeader.sourceNodeID = temp.nodeid;
			newControlHeader.destNodeID = reciever;
			newControlHeader.packetID = packetCounter;
			newControlHeader.TTL = 15;
			host = temp.hostName;
			break;
		}
	}
	newControlPayload.pathToTravel[0] = sender;
	newControlPayload.pathToTravel[1] = reciever;
	//step 3 send that shit
	// now with UDP datagram sockets:
	//getaddrinfo(...
	
	//dest = ...  // assume "dest" holds the address of the destination
	//dgram_socket = socket(...

	// send secret message normally:
	sendto(dgram_socket, secret_message, strlen(secret_message)+1, 0, (struct sockaddr*)&dest, sizeof dest);
}

int createLink(int node1, int node2){
	
}

int removeLink(int node1, int node2){

}

int initialize(string file){
	vector<node>nodeList;
	ifstream inFile;
	inFile.open(file);
	if(!inFile){
		cerr<<"Unable to open file "<<argv[1]<<endl;
		return -1;
	}
	string line;
	while(getline(inFile, line)){
		//instantiate a class first?
		istringstream in(line);
		in>>nodeID>>hostName>>ctrlPort>>dataPort;
		node node;
		node.nodeid = nodeID;
		node.ctrlPort = ctrlPort;
		node.dataPort = dataPort;
		nodeList.push_back(node);
	}
	inFile.close();
	return nodeList;
}


