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

#define PACKET_SIZE 1024

using namespace std;

int packetCounter = 0;
int udpSocket;

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
	inet_pton(AF_INET, INADDR_ANY, &localAddr.sin_addr);

	bind(udpSocket, (struct sockaddr*)&localAddr, sizeof localAddr);

	while(1){
		cin >> command >> node1 >> node2;
		if(command == "generate-packet"){
			if(generatePacket(node1,node2) == -1){
				cout <<"Error sending packet"<<endl;
				exit(1);
			}
		}else if(command == "create-link"){
			if(createLink(node1,node2) == -1){
				cout << "Error creating link" << endl;
				exit(1);
			}
		}else if(command == "remove-Link"){
			if(removeLink(node1,node2)==-1){
				cout << "Error removing link" << endl;
				exit(1);
			}	
		}
	}

}

int generatePacket(int sender,int reciever,nodeList){
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
			packetCounter++;
			newControlHeader.TTL = 15;
			host = sendingNode.hostName;
			break;
		}
	}
	//newControlPayload.pathToTravel[0] = sender;
	//newControlPayload.pathToTravel[1] = reciever;
	//step 3 send that shit

	he = gethostbyname(sendingNode.hostName.c_str());
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(sendingNode.ctrlPort); //dest ctrl port
	memcpy(&destAddr.sin_addr, he->h_addr, he->h_length); //dest ip	

	char packet[PACKET_SIZE];
	memcpy(packet, &header, sizeof(header));

	int result = sendto(udpSocket, packet, PACKET_SIZE, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
	if(result == -1){
		cerr<<"Error sending packet"<<endl;
		cout<<strerror(errno)<<endl;
		exit(1);
	}
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



