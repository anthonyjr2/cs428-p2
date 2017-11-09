#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <arpa/inet.h>

#define PACKET_SIZE 1024
#define DATA_MESSAGE 2
#define ADD_LINK 3
#define DELETE_LINK 4

using namespace std;

int packetCounter = 0;
int udpSocket;

class node{
	public:
		int nodeid, ctrlPort, dataPort;
		string hostName;
	private:
};

typedef struct{
	uint8_t sourceNodeID;
	uint8_t destNodeID;
	uint8_t packetID;
	uint8_t type;
}controlPacketHeader;

typedef struct{
	int pathToTravel[2];
}controlPacketPayload;

typedef struct{
	uint8_t sourceNodeID;
	uint8_t destNodeID;
	uint8_t packetID;
	uint8_t TTL;
}dataPacketHeader;

typedef struct{
	int pathTraveled[100];
}dataPacketPayload;

int generatePacket(int sender,int reciever,vector<node>nodeList);
int createLink(int node1, int node2,vector<node>nodeList);
int removeLink(int node1, int node2,vector<node>nodeList);
vector<node> initialize(string file);

int main(int argc, char *argv[]){
	
	
	if(argc < 2){
		cout<<"usage : "<<argv[0]<<" config.txt"<<endl;
		exit(1);
	}
	vector<node>nodeList = initialize(argv[1]);
	string command;
	int node1, node2;
	struct sockaddr_in localAddr;

	udpSocket = socket(AF_INET, SOCK_DGRAM, 0); //open a UDP socket for all connections
	if(udpSocket == -1){
		cerr<<strerror(errno)<<endl;
		exit(1);
	}

	
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(0);
	//inet_pton(AF_INET, INADDR_ANY, &localAddr.sin_addr);
	localAddr.sin_addr.s_addr = INADDR_ANY;

	bind(udpSocket, (struct sockaddr*)&localAddr, sizeof localAddr);

	while(1){
		cin >> command >> node1 >> node2;
		if(command == "generate-packet"){
			if(generatePacket(node1,node2,nodeList) == -1){
				cout <<"Error sending packet"<<endl;
				exit(1);
			}
		}else if(command == "create-link"){
			if(createLink(node1,node2,nodeList) == -1){
				cout << "Error creating link" << endl;
				exit(1);
			}
		}else if(command == "remove-Link"){
			if(removeLink(node1,node2,nodeList)==-1){
				cout << "Error removing link" << endl;
				exit(1);
			}	
		}else{
			cout<<"Invalid command"<<endl;
		}
	}

}

int generatePacket(int sender,int reciever,vector<node>nodeList){
	string host;
	struct sockaddr_in destAddr;
	struct hostent *he;
	struct in_addr **addr_list;
	//step 1 create control packet
	controlPacketHeader newControlHeader;
	//controlPacketPayload newControlPayLoad;
	node sendingNode;

	//step 2 find the info for the sender packet
	for(int i = 0; i < nodeList.size();i++){
		if(nodeList[i].nodeid == sender){
			//we have found our man
			sendingNode = nodeList[i];
			newControlHeader.sourceNodeID = sender;
			newControlHeader.destNodeID = reciever;
			newControlHeader.packetID = packetCounter;
			newControlHeader.type = DATA_MESSAGE;
			packetCounter++;
			host = sendingNode.hostName;
			break;
		}
	}
	//newControlPayload.pathToTravel[0] = sender;
	//newControlPayload.pathToTravel[1] = reciever;
	//dont think we need a payload here because all the needed info is in the header
	//step 3 send that shit

	he = gethostbyname(sendingNode.hostName.c_str());
	if(he == NULL){
		cerr<<"Error with gethostbyname in send"<<endl;
		cerr<<h_errno<<endl;
		exit(1);
	}
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(sendingNode.ctrlPort); //dest ctrl port
	memcpy(&destAddr.sin_addr, he->h_addr, he->h_length); //dest ip	

	char packet[PACKET_SIZE];
	memcpy(packet, &newControlHeader, sizeof(newControlHeader));

	int result = sendto(udpSocket, packet, PACKET_SIZE, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
	if(result == -1){
		cerr<<"Error sending packet"<<endl;
		cout<<strerror(errno)<<endl;
		exit(1);
	}
}

int createLink(int node1, int node2,vector<node>nodeList){
	//tell nodes to get info from the config table
	//send one message to each node, telling it to look at the other's information 
	string host;
	struct sockaddr_in destAddr;
	struct hostent *he;
	struct in_addr **addr_list;
	//step 1 create control packet
	controlPacketHeader newControlHeader;
	node sendingNode1, sendingNode2;
	//step 2 find the info for the sender packet
	for(int i = 0; i < nodeList.size();i++){
		if(nodeList[i].nodeid == node1){
			//we have found our man
			sendingNode1 = nodeList[i];
			newControlHeader.sourceNodeID = node1;
			newControlHeader.destNodeID = node2;
			newControlHeader.packetID = packetCounter;
			newControlHeader.type = ADD_LINK;
			packetCounter++;
			host = sendingNode1.hostName;
			break;
		}
	}

	/*for(int i = 0; i < nodeList.size();i++){
		if(nodeList[i].nodeid == node2){
			//we have found our man
			sendingNode2 = nodeList[i];
			newControlHeader.sourceNodeID = sender;
			newControlHeader.destNodeID = reciever;
			newControlHeader.packetID = packetCounter;
			newControlHeader.type = ADD_LINK;
			packetCounter++;
			newControlHeader.TTL = 15;
			host = sendingNode.hostName;
			break;
		}
	}*/

	he = gethostbyname(sendingNode1.hostName.c_str());
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(sendingNode1.ctrlPort); //dest ctrl port
	memcpy(&destAddr.sin_addr, he->h_addr, he->h_length); //dest ip	

	char packet[PACKET_SIZE];
	memcpy(packet, &newControlHeader, sizeof(newControlHeader));

	int result = sendto(udpSocket, packet, PACKET_SIZE, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
	if(result == -1){
		cerr<<"Error sending packet"<<endl;
		cout<<strerror(errno)<<endl;
		exit(1);
	}
}

int removeLink(int node1, int node2,vector<node>nodeList){
	//tell nodes to get info from the config table
	//send one message to each node, telling it to look at the other's information 
	string host;
	struct sockaddr_in destAddr;
	struct hostent *he;
	struct in_addr **addr_list;
	//step 1 create control packet
	controlPacketHeader newControlHeader;
	node sendingNode1, sendingNode2;
	//step 2 find the info for the sender packet
	for(int i = 0; i < nodeList.size();i++){
		if(nodeList[i].nodeid == node1){
			//we have found our man
			sendingNode1 = nodeList[i];
			newControlHeader.sourceNodeID = node1;
			newControlHeader.destNodeID = node2;
			newControlHeader.packetID = packetCounter;
			newControlHeader.type = DELETE_LINK;
			packetCounter++;
			host = sendingNode1.hostName;
			break;
		}
	}

	he = gethostbyname(sendingNode1.hostName.c_str());
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(sendingNode1.ctrlPort); //dest ctrl port
	memcpy(&destAddr.sin_addr, he->h_addr, he->h_length); //dest ip	

	char packet[PACKET_SIZE];
	memcpy(packet, &newControlHeader, sizeof(newControlHeader));

	int result = sendto(udpSocket, packet, PACKET_SIZE, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
	if(result == -1){
		cerr<<"Error sending packet"<<endl;
		cout<<strerror(errno)<<endl;
		exit(1);
	}
}

vector<node> initialize(string file){
	int nodeID, ctrlPort, dataPort, packetIDCtr, udpSocket;
	string hostName;
	vector<node>nodeList;
	ifstream inFile;
	inFile.open(file);
	if(!inFile){
		cerr<<"Unable to open file "<<file<<endl;
		//return -1;
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
		node.hostName = hostName;
		nodeList.push_back(node);
	}
	inFile.close();
	return nodeList;
}



