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

#define PACKET_SIZE 1024

using namespace std;

typedef chrono::high_resolution_clock Clock;
typedef chrono::milliseconds ms;

//routing table 
typedef struct{
	int intermediateNode;
	int distance;
}routeStruct;

typedef struct{
	uint8_t sourceNodeID;
	uint8_t destNodeID;
	uint8_t packetID;
	int otherSize;
}packetHeader;

typedef struct
{
	int ctrlPort;
	int dataPort;
	string hostName;
}nodeStruct;

typedef struct{
	uint8_t sourceNodeID;
	uint8_t destNodeID;
	uint8_t packetID;
	uint8_t TTL;
}controlPacketHeader;

typedef struct{
	int pathToTravel[2];
}controlPacketPayload;

int nodeID, ctrlPort, dataPort, packetIDCtr, udpSocket;
string hostName;
vector<int> neighbors;
double difference;

map<int,routeStruct> routingTable;
map<int, nodeStruct> configTable;

void sendDistanceVector(int destNodeID);
void receiveDistanceVector();
void updateRoutingTable(int destNodeID, int sourceNodeID, int senderPort, map<int, routeStruct> recvdRoutingTable);
static double diffclock(clock_t clock1,clock_t clock2);

int main(int argc, char *argv[])
{
	int thisNodeID = atoi(argv[2]);
	
	if(argc != 3)
	{
		cout<<"usage : "<<argv[0]<<" config.txt <nodeID>"<<endl;
		exit(1);
	}
	
	ifstream inFile;
	inFile.open(argv[1]);
	if(!inFile)
	{
		cerr<<"Unable to open file "<<argv[1]<<endl;
		exit(1);
	}
	string line;
	while(getline(inFile, line))
	{
		istringstream in(line);
		in>>nodeID>>hostName>>ctrlPort>>dataPort;
		
		nodeStruct ns;
		ns.ctrlPort = ctrlPort;
		ns.dataPort = dataPort;
		ns.hostName = hostName;
		configTable.insert(pair<int, nodeStruct> (nodeID, ns));
		
		if(nodeID == thisNodeID)
		{
			int n;
			while(in>>n)
			{
				routeStruct neighbor;
				neighbor.distance = 1;
				neighbor.intermediateNode = thisNodeID;
				routingTable.insert(pair <int, routeStruct> (n, neighbor));
				neighbors.push_back(n);
			}
		}
	}
	routeStruct thisNode;
	thisNode.distance = -1;
	thisNode.intermediateNode = thisNodeID;
	routingTable.insert(pair<int,routeStruct>(thisNodeID, thisNode));
	inFile.close();
	
	struct sockaddr_in localAddr;
	
	udpSocket = socket(AF_INET, SOCK_DGRAM, 0); //udp socket open
	if(udpSocket == -1)
	{
		cerr<<strerror(errno)<<endl;
		exit(1);
	}
	
	memset((char*)&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(ctrlPort);
	localAddr.sin_addr.s_addr = INADDR_ANY;
	
	int result = bind(udpSocket, (struct sockaddr*)&localAddr, sizeof(localAddr));
	if(result < 0)
	{
		cerr<<"Bind on local socket for sending distance vector failed"<<endl;
		cerr<<strerror(errno)<<endl;
		exit(1);
	}
	
	auto start = Clock::now();
	while(1)
	{
		auto diff = chrono::duration_cast<ms>(Clock::now() - start);
		if(diff.count() > 2000)
		{	
			for(int i = 0; i < neighbors.size(); i++)
			{
				cout<<"Sending packet from node "<<thisNodeID<<" to node "<<neighbors.at(i)<<endl;
				sendDistanceVector(neighbors.at(i));
			}
			start = Clock::now();
		}
		receiveDistanceVector();
	}
}

void sendDistanceVector(int destNodeID){
	struct sockaddr_in destAddr;
	struct hostent *he;
	struct in_addr **addr_list;
	
	he = gethostbyname(configTable[destNodeID].hostName.c_str());
	if(he == NULL)
	{
		cerr<<"Error with gethostbyname in send"<<endl;
		cerr<<h_errno<<endl;
	}
	
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(configTable[destNodeID].ctrlPort); //dest ctrl port and ip
	memcpy(&destAddr.sin_addr, he->h_addr, he->h_length);
	
	packetHeader header;
	header.sourceNodeID = nodeID;
	header.destNodeID = destNodeID;
	header.packetID = packetIDCtr;
	
	int distanceToSend[routingTable.size()+1];
	int nodesToSend[routingTable.size()+1];
	int destinationsToSend[routingTable.size()+1];
	for(int i = 0; i < routingTable.size(); i++)
	{
		nodesToSend[i] = routingTable[i].intermediateNode;
	}
	for(int i = 0; i < routingTable.size(); i++)
	{
		distanceToSend[i] = routingTable[i].distance;
	}
	for(int i = 0; i < routingTable.size(); i++)
	{
		destinationsToSend[i] = routingTable[i].distance;
	}
	nodesToSend[-1] = -1;
	distanceToSend[-1] = -1;
	destinationsToSend[-1] = -1;
	
	char packet[PACKET_SIZE];
	memcpy(packet, &header, sizeof(header));
	memcpy(packet + sizeof(header), &nodesToSend, sizeof(nodesToSend));
	memcpy(packet + sizeof(header) + sizeof(nodesToSend), &distanceToSend, sizeof(distanceToSend));
	memcpy(packet + sizeof(header) + sizeof(nodesToSend) + sizeof(distanceToSend), &destinationsToSend, sizeof(destinationsToSend));

	int result = sendto(udpSocket, packet, PACKET_SIZE, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
	if(result == -1)
	{
		cerr<<"Error sending packet"<<endl;
		cout<<strerror(errno)<<endl;
		exit(1);
	}
}
void receiveDistanceVector(){
	struct hostent *he;
	struct sockaddr_in senderAddr;
	socklen_t fromlen;
	struct timeval tv;
	
	fd_set readfds; //fd_set for select
	FD_ZERO(&readfds);
	FD_SET(udpSocket, &readfds);
	
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	int rv = select(udpSocket + 1, &readfds, NULL, NULL, &tv);
	if(rv == -1)
	{
		cerr<<"Select error"<<endl;
	}
	else if(rv == 0)
	{
		cout<<"Timeout on receving packet"<<endl;
	}
	else
	{
		char buf[512];
		fromlen = sizeof(senderAddr);
		int result = recvfrom(udpSocket, buf, sizeof(buf), 0, (struct sockaddr*)&senderAddr, &fromlen);
		if(result == -1)
		{
			cerr<<"Error recieving packet"<<endl;
			cout<<strerror(errno)<<endl;
			exit(1);
		}
		int senderPort = ntohs(senderAddr.sin_port);
	
		//should receive the routingTable from the sender in buf along with packet header
	
		char recvdPacketHeader[sizeof(packetHeader)];
		memcpy(recvdPacketHeader, buf, sizeof(packetHeader));
		packetHeader p;
		memcpy(&p, recvdPacketHeader, sizeof(packetHeader));
	
		char recvdTableBuffer[PACKET_SIZE - sizeof(packetHeader)];
		memcpy(recvdTableBuffer, buf + sizeof(packetHeader), sizeof(recvdTableBuffer));
		map<int,routeStruct> recvdRoutingTable;
	
		int recvdDist[512];
		int recvdIntNode[512];
		int recvdDestNode[512];
	
		int ctr = 0;
		for(int i = 0; i < PACKET_SIZE - sizeof(packetHeader); i++)
		{
			if(recvdTableBuffer[i] != -1)
			{
				recvdIntNode[i] = recvdTableBuffer[i];
				ctr++;
			}
			else
			{
				for(int k = i+1; k < PACKET_SIZE - sizeof(packetHeader); k++)
				{
					if(recvdTableBuffer[k] != -1)
					{
						recvdDist[k] = recvdTableBuffer[k];
					}
					else
					{
						for(int j = k+1; j < PACKET_SIZE - sizeof(packetHeader); j++)
						{
							if(recvdTableBuffer[j] != -1)
							{
								recvdDestNode[j] = recvdTableBuffer[j];
							}
							else
							{
								break;
							}
						}
						break;
					}
				}
				break;
			}
		}

		for(int i = 0; i < ctr; i++)
		{
			routeStruct recvdStruct;
			recvdStruct.intermediateNode = recvdIntNode[i];
			recvdStruct.distance = recvdDist[i]; 
			recvdRoutingTable.insert(pair<int,routeStruct>(recvdDestNode[i],recvdStruct));
		}
	
		updateRoutingTable(p.destNodeID,p.sourceNodeID,senderPort, recvdRoutingTable);
	}
}

void updateRoutingTable(int destNodeID, int sourceNodeID, int senderPort, map<int, routeStruct> recvdRoutingTable)
{
	//case 1: node in recvd routing table is not in this nodes routing table
	map<int,routeStruct>::iterator it;
	map <int, routeStruct>::iterator iter;
	for(iter= recvdRoutingTable.begin();iter != recvdRoutingTable.end();iter++){
		it = routingTable.find(iter->first);
		if (it == routingTable.end()){
			//element is not in this nodes routing table
			//there is a new node to put into our map
			routeStruct case1RouteStruct;
			case1RouteStruct.distance = iter->second.distance+1;
			case1RouteStruct.intermediateNode = iter->second.intermediateNode;
			routingTable.insert(pair<int,routeStruct>(iter->first,case1RouteStruct));
		}
	}
	//case 2: there is more optimal route than we have stored in this routing table
	for(iter = recvdRoutingTable.begin(); iter != recvdRoutingTable.end();iter++){
		it = routingTable.find(iter->first);
		if (it != routingTable.end()){
			if(it->second.distance < iter->second.distance+1){
				it->second.distance = iter->second.distance+1;
				it->second.intermediateNode = iter->second.intermediateNode;
			}
		}
	}
	//case 3: distance vector coming in on same port so we have to update
	if(senderPort == ctrlPort){
		for(iter = recvdRoutingTable.begin(); iter != recvdRoutingTable.end();iter++){
			it = routingTable.find(iter->first);
			if (it != routingTable.end()){
				if(it->first != nodeID){
					it->second.distance = iter->second.distance+1;
					it->second.intermediateNode = iter->second.intermediateNode;
				}
			}		
		}
	}
}




