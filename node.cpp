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
#include <ctime>

#define PACKET_SIZE 1024

using namespace std;

//routing table 
typedef struct{
	int intermediateNode;
	//int lastTraveledNode;
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

int nodeID, ctrlPort, dataPort, packetIDCtr, udpSocket;
string hostName;
vector<int> neighbors;
clock_t start;
double difference;

map<int,routeStruct> routingTable;
map<int, nodeStruct> configTable;

void sendDistanceVector(int destNodeID);
void receiveDistanceVector();
void updateRoutingTable(int nodeID, map<int, routeStruct> recvdRoutingTable);
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
	thisNode.distance = -1
	thisNode.intermediateNode = thisNodeID;
	routingTable.insert(pair<int,routeStruct>(thisNodeID, thisNode);
	inFile.close();
	
	struct sockaddr_in localAddr;
	
	udpSocket = socket(AF_INET, SOCK_DGRAM, 0); //udp socket open
	if(udpSocket == -1)
	{
		cerr<<strerror(errno)<<endl;
		exit(1);
	}
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
	
	start = clock();
	while(1)
	{
		receiveDistanceVector();
		if(diffclock(clock(), start) > 2000)
		{
			for(int i = 0; i < neighbors.size(); i++)
			{
				sendDistanceVector(neighbors.at(i));
			}
			start = clock();
		}
	}
}

static double diffclock(clock_t clock1,clock_t clock2)
{
    double diffticks=clock1-clock2;
    double diffms=(diffticks)/(CLOCKS_PER_SEC/1000);
    return diffms;
}

void sendDistanceVector(int destNodeID){
	int udpSocket;
	struct sockaddr_in destAddr;
	struct hostent *he;
	struct in_addr **addr_list;
	
	char portbuf[7];
	snprintf(portbuf, sizeof(portbuf), "%d", configTable[destNodeID].ctrlPort);
	he = gethostbyname(configTable[destNodeID].hostName.c_str());
	
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(configTable[destNodeID].ctrlPort); //dest ctrl port and ip
	memcpy(&destAddr.sin_addr, he->h_addr, he->h_length);
	
	packetHeader header;
	header.sourceNodeID = nodeID;
	header.destNodeID = destNodeID;
	header.packetID = packetIDCtr;
	
	int distanceToSend[routingTable.size()+1];
	int nodesToSend[routingTable.size()+1];

	for(int i = 0; i < routingTable.size()+1; i++)
	{
		nodesToSend[i] = routingTable[i].destinationNode;
	}
	for(int i = 0; i < routingTable.size()+1; i++)
	{
		distanceToSend[i] = routingTable[i].distance;
	}
	nodesToSend[-1] = -1;
	distanceToSend[-1] = -1;
	
	char packet[PACKET_SIZE];
	memcpy(packet, &header, sizeof(header));
	memcpy(packet + sizeof(header), &nodesToSend, sizeof(nodesToSend));
	memcpy(packet + sizeof(header) + sizeof(nodesToSend), &distanceToSend, sizeof(distanceToSend));
	
	sendto(udpSocket, packet, PACKET_SIZE, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));

}

void receiveDistanceVector(){
	struct addrinfo receiveInfo, *senderInfo;
	struct sockaddr_in senderAddr;
	socklen_t fromlen;
	int sockfd;

	receiveInfo.ai_family = AF_INET;
	receiveInfo.ai_socktype = SOCK_DGRAM;
	receiveInfo.ai_flags = AI_PASSIVE;
	
	char portbuf[7];
	snprintf(portbuf, sizeof(portbuf), "%d", ctrlPort);
	
	memset(&receiveInfo, 0, sizeof(receiveInfo));
	getaddrinfo(NULL, portbuf, &receiveInfo, &senderInfo);

	char buf[512];
	fromlen = sizeof(senderAddr);
	int result = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&senderAddr, &fromlen);
	if(result == -1)
	{
		//cerr<<"Error recieving packet"<<endl;
		//cout<<strerror(errno)<<endl;
	}
	
	//should receive the routingTable from the sender in buf along with packet header
	
	char recvdPacketHeader[sizeof(packetHeader)];
	memcpy(recvdPacketHeader, buf, sizeof(packetHeader));
	packetHeader p;
	memcpy(&p, recvdPacketHeader, sizeof(packetHeader));
	
	char recvdTableBuffer[PACKET_SIZE - sizeof(packetHeader)];
	memcpy(recvdTableBuffer, buf + sizeof(packetHeader), sizeof(recvdTableBuffer));
	map<int,routeStruct> recvdRoutingTable;
	
	int recvdDist[512];
	int recvdDestNode[512];
	
	int ctr = 0;
	for(int i = 0; i < PACKET_SIZE - sizeof(packetHeader); i++)
	{
		if(recvdTableBuffer[i] != -1)
		{
			recvdDestNode[i] = recvdTableBuffer[i];
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
					break;
				}
			}
			break;
		}
	}

	for(int i = 0; i < ctr; i++)
	{
		routeStruct recvdStruct;
		recvdStruct.destinationNode = recvdDestNode[i];
		recvdStruct.distance = recvdDist[i]; 
		recvdRoutingTable.insert(pair<int,routeStruct>(p.sourceNodeID,recvdStruct));
	}
	
	
	updateRoutingTable(p.destNodeID,p.sourceNodeID, recvdRoutingTable);
}

void updateRoutingTable(int nodeID, int sourceID map<int, routeStruct> recvdRoutingTable)
{
	//case 1: node in recvd routing table is not in this nodes routing table
	map<int,routeStruct>::iterator it;
	map <int, routeStruct>::iterator iter;
	for(iter= recvdRoutingTable.begin();iter != recvdRoutingTable.end();iter++){P
		it = routingTable.find(i);
		if (it == routingTable.end()){
			//found element
			//there is a new node to put into our map
			routeStruct case1RouteStruct;
			case1RouteStruct.distance = iter.distance+1;
			case1RouteStruct.intermediateNode = iter.intermediateNode;
			routingTable.insert(pair<int,routeStruct>(iter->first,case1RouteStruct));
		}
	}
	//case 2: there is more optimal route than we have stored in this routing table
	
	//case 3: 


}




