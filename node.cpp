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
#include <mutex>
#include <climits>

#define PACKET_SIZE 1024
#define DATA_MESSAGE 2
#define ADD_LINK 3
#define DELETE_LINK 4

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
	uint8_t type;
	uint8_t TTL;
}packetHeader;

typedef struct
{
	int ctrlPort;
	int dataPort;
	string hostName;
}nodeStruct;

int nodeID, ctrlPort, dataPort, packetIDCtr, controlSocket, dataSocket, controlDestinationNode;
string hostName;
vector<int> neighbors;

bool dataToSend = false;

mutex sendFlagMutex;
mutex routingLock;

map<int,routeStruct> routingTable;
map<int, nodeStruct> configTable;

void sendDistanceVector(int destNodeID);
void receiveDistanceVector();
void updateRoutingTable(int destNodeID, int sourceNodeID, int senderPort, map<int, routeStruct> recvdRoutingTable);
void sendDataPacket(char packet[PACKET_SIZE]);
void receiveDataPacket();
void startControlThread();
void startDataThread();
void buildDataPacket(int destNodeID);

int main(int argc, char *argv[])
{	
	if(argc != 3)
	{
		cout<<"usage : "<<argv[0]<<" config.txt <nodeID>"<<endl;
		exit(1);
	}
	int thisNodeID = atoi(argv[2]);
	
	ifstream inFile;
	inFile.open(argv[1]);
	if(!inFile)
	{
		cerr<<"Unable to open file "<<argv[1]<<endl;
		exit(1);
	}
	string line;
	while(getline(inFile, line)) //get each line of the config
	{
		istringstream in(line);
		in>>nodeID>>hostName>>ctrlPort>>dataPort;
		
		nodeStruct ns; //instantiate a struct with the data we read so we have the config for later
		ns.ctrlPort = ctrlPort;
		ns.dataPort = dataPort;
		ns.hostName = hostName;
		configTable.insert(pair<int, nodeStruct> (nodeID, ns));
		
		if(nodeID == thisNodeID) //if the line we are reading is our info
		{
			int n;
			while(in>>n)
			{
				routeStruct neighbor; //add the neighbors in that line to current node's routing table
				neighbor.distance = 1; //neighbors have 1 distance
				neighbor.intermediateNode = thisNodeID;
				routingTable.insert(pair <int, routeStruct> (n, neighbor));
				neighbors.push_back(n); //also add to our neighbors list to make it easier to send distance vectors
			}
		}
	}
	routeStruct thisNode; //insert ourselves into the routing table
	thisNode.distance = 0;
	thisNode.intermediateNode = thisNodeID;
	routingTable.insert(pair<int,routeStruct>(thisNodeID, thisNode));
	inFile.close();
	
	nodeID = thisNodeID;
	ctrlPort = configTable[nodeID].ctrlPort;
	dataPort = configTable[nodeID].dataPort; //set global variables to this node's info
	hostName = configTable[nodeID].hostName;
	
	controlSocket = socket(AF_INET, SOCK_DGRAM, 0); //open a UDP socket for all connections
	if(controlSocket == -1)
	{
		cerr<<strerror(errno)<<endl;
		exit(1);
	}
	
	struct hostent *he;
	he = gethostbyname(hostName.c_str());
	if(he == NULL)
	{
		cerr<<"Error with gethostbyname in send"<<endl;
		cerr<<h_errno<<endl;
		exit(1);
	}
	
	struct sockaddr_in localAddr;
	memset((char*)&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(ctrlPort);
	memcpy(&localAddr.sin_addr, he->h_addr, he->h_length);
	
	//bind the port to the socket we opened earlier
	int result = bind(controlSocket, (struct sockaddr*)&localAddr, sizeof(localAddr));
	if(result < 0)
	{
		cerr<<"Bind on local socket for sending distance vector failed"<<endl;
		cerr<<strerror(errno)<<endl;
		exit(1);
	}
	
	dataSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(dataSocket == -1)
	{
		cerr<<strerror(errno)<<endl;
		exit(1);
	}
	
	struct sockaddr_in localAddrData;
	memset((char*)&localAddrData, 0, sizeof(localAddrData));
	localAddrData.sin_family = AF_INET;
	localAddrData.sin_port = htons(dataPort);
	memcpy(&localAddrData.sin_addr, he->h_addr, he->h_length);
	
	//bind the port to the socket we opened earlier
	result = bind(dataSocket, (struct sockaddr*)&localAddrData, sizeof(localAddrData));
	if(result < 0)
	{
		cerr<<"Bind on local socket for sending data packets failed"<<endl;
		cerr<<strerror(errno)<<endl;
		exit(1);
	}
	
	thread cntrlThread(startControlThread);
	thread dataThread(startDataThread);
	
	cntrlThread.join();
	dataThread.join();

}

void startControlThread()
{
	auto start = Clock::now(); //start a clock for sending
	while(1)
	{
		auto diff = chrono::duration_cast<ms>(Clock::now() - start);
		if(diff.count() > 2000) //send every 2s
		{
			for(int i = 0; i < neighbors.size(); i++)
			{
				//cout<<"["<<packetIDCtr<<"]"<<"Sending packet from node "<<nodeID<<" to node "<<neighbors.at(i)<<endl;
				sendDistanceVector(neighbors.at(i));
			}
			start = Clock::now();
		}
		receiveDistanceVector(); //receive distance vector if any are pending
	}
}

void startDataThread()
{
	while(1)
	{
		sendFlagMutex.lock();
		if(dataToSend) //check global flag to see if we have any data to send
		{
			dataToSend = false;
			buildDataPacket(controlDestinationNode); //need to also check for send when we receive a data packet that needs to be forwarded
		}
		sendFlagMutex.unlock();
		receiveDataPacket(); //receive data packet if any are pending
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
		exit(1);
	}
	
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(configTable[destNodeID].ctrlPort); //dest ctrl port
	memcpy(&destAddr.sin_addr, he->h_addr, he->h_length); //dest ip
	
	packetHeader header;
	header.sourceNodeID = nodeID;
	header.destNodeID = destNodeID;
	header.packetID = packetIDCtr;
	
	char distanceToSend[(PACKET_SIZE - sizeof(header))/2];
	char destinationsToSend[(PACKET_SIZE - sizeof(header))/2];
	
	
	int i = 0;
	map <int, routeStruct>::iterator iter;
	routingLock.lock();
	for(iter= routingTable.begin();iter != routingTable.end();iter++)
	{
		distanceToSend[i] = iter->second.distance;
		i++;
	}
	distanceToSend[i] = -1;

	i = 0;
	for(iter= routingTable.begin();iter != routingTable.end();iter++)
	{
		destinationsToSend[i] = iter->first;
		i++;
	}
	routingLock.unlock();
	destinationsToSend[i] = -1;
	
	char packet[PACKET_SIZE];
	memcpy(packet, &header, sizeof(header));
	memcpy(packet + sizeof(header), &distanceToSend, sizeof(distanceToSend));
	memcpy(packet + sizeof(header) + sizeof(distanceToSend), &destinationsToSend, sizeof(destinationsToSend));

	int result = sendto(controlSocket, packet, PACKET_SIZE, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
	if(result == -1)
	{
		cerr<<"Error sending packet"<<endl;
		cout<<strerror(errno)<<endl;
		exit(1);
	}
	//packetIDCtr++;
}
void receiveDistanceVector(){
	struct hostent *he;
	struct sockaddr_in senderAddr;
	socklen_t fromlen;
	struct timeval tv;
	
	fd_set readfds; //fd_set for select
	FD_ZERO(&readfds);//reset the fd set each time so it works
	FD_SET(controlSocket, &readfds);
	
	tv.tv_sec = 3;//timeout is 3 seconds
	tv.tv_usec = 0;
	int rv = select(controlSocket + 1, &readfds, NULL, NULL, &tv);//blocking and waiting for input from udp
	if(rv == -1)
	{
		cerr<<"Select error"<<endl;//error
	}
	else if(rv == 0)
	{
		//cout<<"[ERROR]Timeout"<<endl;//timeout
	}
	else if(FD_ISSET(controlSocket, &readfds))//detected something from controlSocket
	{
		char buf[PACKET_SIZE];
		fromlen = sizeof(senderAddr);
		int result = recvfrom(controlSocket, buf, PACKET_SIZE, 0, (struct sockaddr*)&senderAddr, &fromlen);
		if(result == -1)//result is how many bits the revieve got from the sender
		{
			cerr<<"Error recieving packet"<<endl;
			cout<<strerror(errno)<<endl;
			exit(1);
		}
		int senderPort = ntohs(senderAddr.sin_port);
	
		//should receive the routingTable from the sender in buf along with packet header
	
		//taking the header out of the buffer and putting it into a char array
		char recvdPacketHeader[sizeof(packetHeader)];
		memcpy(recvdPacketHeader, buf, sizeof(packetHeader));
		packetHeader p;
		memcpy(&p, recvdPacketHeader, sizeof(packetHeader));
		
		if(p.type == DATA_MESSAGE)
		{
			if(nodeID == p.destNodeID)
			{
				//we're done, print the payload
				//display packet info
				cout<<"Packet received, forwarding table: ";
				cout<<nodeID<<" "<<unsigned(p.destNodeID);
				cout<<endl<<"End of path."<<endl;
			}
			else
			{
				controlDestinationNode = p.destNodeID;
				//set flag to send stuff out now
				sendFlagMutex.lock();
				dataToSend = true;
				sendFlagMutex.unlock();
			}
		}
		else if(p.type == ADD_LINK)
		{
			routingLock.lock();
			routeStruct s;
			s.intermediateNode = p.sourceNodeID;
			s.distance = 1;
			routingTable.erase(p.destNodeID);
			routingTable.insert(pair<int,routeStruct>(p.destNodeID,s));
			cout<<"New routing table for node "<<unsigned(p.sourceNodeID)<<" after create:"<<endl;
			for(auto it = routingTable.cbegin(); it != routingTable.cend(); ++it)
			{
<<<<<<< HEAD
				cout<<it->first<<" "<<it->second.intermediateNode<<" "<< it->second.distance<<"\n";
=======
				cout<<it->first<< " " <<it->second.intermediateNode<<" "<< it->second.distance<<"\n";
>>>>>>> 2aba4e0df1e4745563425ffee00367bf01f33ef1
			}
			neighbors.push_back(p.destNodeID);
			routingLock.unlock();
		}
		else if(p.type == DELETE_LINK)
		{
			routingLock.lock(); 	
			if(routingTable.find(p.destNodeID) == routingTable.end())
			{
				cout<<"Node "<<unsigned(nodeID)<<" does not have node "<<unsigned(p.destNodeID)<<" in its routing table to delete"<<endl;
			}
			else
			{
<<<<<<< HEAD
=======
				//link removed
				routingTable[p.destNodeID].distance = INT_MAX;
				
>>>>>>> 2aba4e0df1e4745563425ffee00367bf01f33ef1
				for(int i = 0; i < neighbors.size(); i++)
				{
					if(p.destNodeID == neighbors[i])
					{
						neighbors.erase(neighbors.begin()+i);
						break;
					}
				} 				
				for(auto it = routingTable.begin(); it != routingTable.end(); ++it)
  				{
	  				if(it->second.intermediateNode == p.destNodeID)
	  				{
	 					routingTable.erase(it);
	 				}
  				}
  				//link removed
				cout<<"New routing table for node "<<nodeID<<" after remove:"<<endl;
				for(auto it = routingTable.cbegin(); it != routingTable.cend(); ++it)
				{
					std::cout << it->first << " " << it->second.intermediateNode << " " << it->second.distance << "\n";
				}
				for(auto it = routingTable.cbegin(); it != routingTable.cend(); ++it)
				{
					if(it->second.intermediateNode == p.destNodeID){
						routingTable.erase(it);
					}
				}
				cout<<"New routing table for node "<<nodeID<<" after remove:"<<endl;
				for(auto it = routingTable.cbegin(); it != routingTable.cend(); ++it)
				{
					std::cout << it->first << " " << it->second.intermediateNode << " " << it->second.distance << "\n";
				}
			}
			routingLock.unlock();
		}
		else
		{
			char recvdDist[(PACKET_SIZE - sizeof(packetHeader))/2];
			char recvdDestNode[(PACKET_SIZE - sizeof(packetHeader))/2];
			memcpy(recvdDist, buf + sizeof(packetHeader), sizeof(recvdDist));
			memcpy(recvdDestNode, buf + sizeof(packetHeader) + sizeof(recvdDist), sizeof(recvdDestNode));
			map<int,routeStruct> recvdRoutingTable;

			//reconstructing the map
			int i = 0;
			while(recvdDestNode[i] != -1)
			{
				routeStruct recvdStruct;
				recvdStruct.distance = recvdDist[i];
				recvdStruct.intermediateNode = unsigned(p.sourceNodeID);
				recvdRoutingTable.insert(pair<int,routeStruct>(recvdDestNode[i],recvdStruct));
				i++;
			}
		
			//cout<<"["<<packetIDCtr<<"]"<<"Node "<<nodeID<< " received packet from node "<<unsigned(p.sourceNodeID)<<endl;
	
			//update algorithm
			routingLock.lock();
			updateRoutingTable(p.destNodeID,p.sourceNodeID,senderPort, recvdRoutingTable);
			routingLock.unlock();
			
			//print out our updated routing table
			for(auto it = routingTable.cbegin(); it != routingTable.cend(); ++it)
			{
				std::cout << it->first << " " << it->second.intermediateNode << " " << it->second.distance << "\n";
			}
<<<<<<< HEAD
			cout<<endl;*/
=======
			cout<<endl;
>>>>>>> 2aba4e0df1e4745563425ffee00367bf01f33ef1
		}
		
	}
}

void updateRoutingTable(int destNodeID, int sourceNodeID, int senderPort, map<int, routeStruct> recvdRoutingTable)
{
	//case 1: node in recvd routing table is not in this nodes routing table
	map<int,routeStruct>::iterator it;
	map<int,routeStruct>::iterator recievedTableIter;
	map <int, routeStruct>::iterator iter; //received table iterator
	for(iter= recvdRoutingTable.begin();iter != recvdRoutingTable.end();iter++){
		it = routingTable.find(iter->first);
		if (it == routingTable.end()){
			//element is not in this nodes routing table
			//there is a new node to put into our map
			routeStruct case1RouteStruct;
			case1RouteStruct.distance = iter->second.distance+1;
			//case1RouteStruct.intermediateNode = iter->second.intermediateNode;
			case1RouteStruct.intermediateNode = sourceNodeID;
			routingTable.insert(pair<int,routeStruct>(iter->first,case1RouteStruct));
			//neighbors.push_back(destNodeID);
		}
	}
	//case 2: there is more optimal route than we have stored in this routing table
	for(iter = recvdRoutingTable.begin(); iter != recvdRoutingTable.end();iter++){
		it = routingTable.find(iter->first);
		if (it != routingTable.end()){
			if((it->second.distance > iter->second.distance+1) || (it->second.distance == -1)){
				it->second.distance = iter->second.distance+1;
				it->second.intermediateNode = sourceNodeID;
				//it->second.intermediateNode = iter->second.intermediateNode;
			}
		}
	}
<<<<<<< HEAD
	/*//case 3: distance vector coming in on same port so we have to update
	if(senderPort == ctrlPort){
		for(iter = recvdRoutingTable.begin(); iter != recvdRoutingTable.end();iter++){
			it = routingTable.find(iter->first);
			if (it != routingTable.end()){
				if(it->first != nodeID){
					it->second.distance = iter->second.distance+1;
					it->second.intermediateNode = iter->second.intermediateNode;
				}
			}		
=======
	//case 3: force update changes beyond our intermediate node
	
	for(auto ourIter = routingTable.begin(); ourIter != routingTable.end();ourIter++){
		if(ourIter->second.intermediateNode == sourceNodeID){
			it = recvdRoutingTable.find(ourIter->first);
			if(it!= recvdRoutingTable.end()){
				ourIter->second.distance = it->second.distance++;
			}
>>>>>>> 2aba4e0df1e4745563425ffee00367bf01f33ef1
		}
	}*/
	//THIS IS CAUSING DISTANCES TO BE ALL 1
	//case 3: force update changes beyond our intermediate node
	for(auto ourIter = routingTable.begin(); ourIter != routingTable.end();ourIter++){
		if(ourIter->second.intermediateNode == sourceNodeID){
			//recievedTableIter = recvdRoutingTable.find(ourIter->first);
			//if(it!= recvdRoutingTable.end()){
			//	ourIter->second.distance = it->second.distance++;
			//}
			//cout << "Force updating " << nodeID <<"s destination: " << ourIter->first <<" that has a distance of "<< ourIter->second.distance << " with data from node " <<sourceNodeID << " that has distance " << recvdRoutingTable[ourIter->first].distance << endl;
			int incomingDistance = recvdRoutingTable[ourIter->first].distance;
			//cout << "Incoming distance before increment is: " << incomingDistance << endl;
			ourIter->second.distance = ++incomingDistance;
			//ourIter->second.distance = recvdRoutingTable[ourIter->first].distance++;
			//cout <<"The new distance for destination " <<ourIter->first << " is " << ourIter->second.distance<<endl;
			
		}
	}
}

void receiveDataPacket()
{
	struct hostent *he;
	struct sockaddr_in senderAddr;
	socklen_t fromlen;
	struct timeval tv;

	fd_set readfds; //fd_set for select
	FD_ZERO(&readfds);//reset the fd set each time so it works
	FD_SET(dataSocket, &readfds);

	tv.tv_sec = 3;//timeout is 3 seconds
	tv.tv_usec = 0;
	int rv = select(dataSocket + 1, &readfds, NULL, NULL, &tv);//blocking and waiting
	if(rv == -1)
	{
		cerr<<"Select error"<<endl;//error
	}
	else if(rv == 0)
	{
		//cout<<"[ERROR]Timeout"<<endl;//timeout
	}
	else if(FD_ISSET(dataSocket, &readfds))//detected something from dataSocket
	{
		char buf[PACKET_SIZE];
		fromlen = sizeof(senderAddr);
		int result = recvfrom(dataSocket, buf, PACKET_SIZE, 0, (struct sockaddr*)&senderAddr, &fromlen);
		if(result == -1)//result is how many bits the revieve got from the sender
		{
			cerr<<"Error recieving packet"<<endl;
			cout<<strerror(errno)<<endl;
			exit(1);
		}

		packetHeader p;
		memcpy(&p, buf, sizeof(packetHeader));
		
		cout<<"[] Node "<<nodeID<<" received packet from node "<<unsigned(p.sourceNodeID)<<endl;
		
		//check if we are the destination
		if(p.destNodeID == nodeID)
		{
			char payload[PACKET_SIZE - sizeof(packetHeader)];
			memcpy(payload, buf + sizeof(packetHeader), sizeof(payload));
			
			int index = 15 - p.TTL;
			payload[index] = nodeID;
			
			cout<<"Packet received, forwarding table: ";
			cout<<unsigned(p.sourceNodeID)<<" ";
			for(int i = 0; i < index; i++)
			{
				cout<<unsigned(payload[i])<<" ";
			}
			cout<<unsigned(p.destNodeID);
			cout<<endl<<"End of path."<<endl;
		}
		else
		{
		
			if(p.TTL <= 0)
			{
				cout<<"TTL has become 0, destroying packet"<<endl;
			}
			else
			{
				char payload[PACKET_SIZE - sizeof(packetHeader)];
				memcpy(payload, buf + sizeof(packetHeader), sizeof(payload));
			
				int index = 15 - p.TTL;
				payload[index] = nodeID;
			
				char packet[PACKET_SIZE];
				p.TTL--;
				memcpy(packet, &p, sizeof(packetHeader));
				memcpy(packet + sizeof(packetHeader), &payload, sizeof(payload));
				cout<<"[] Node "<<nodeID<<" forwarding packet from node "<<unsigned(p.sourceNodeID)<<" to node "<<unsigned(p.destNodeID)<<endl;
				sendDataPacket(packet);
			}
		}
	}
}

void buildDataPacket(int destNodeID)
{
	//check our routing table for the destination
	//forward it
	routingLock.lock();
	bool res = routingTable.find(destNodeID) == routingTable.end();
	routingLock.unlock();
	if(res)
	{
		cout<<destNodeID<<" is not in this node's routing table"<<endl;
	}
	else
	{
		packetHeader header;
		header.sourceNodeID = nodeID;
		header.destNodeID = destNodeID;
		header.packetID = packetIDCtr++;
		header.TTL = 15;
		
		char payload[PACKET_SIZE - sizeof(header)];

		char packet[PACKET_SIZE];
		memcpy(packet, &header, sizeof(header));
		memcpy(packet + sizeof(header), &payload, sizeof(payload));
		cout<<"[] Node "<<nodeID<<" generated packet to node "<<destNodeID<<endl;
		sendDataPacket(packet);
	}
}

void sendDataPacket(char packet[PACKET_SIZE])
{
	struct sockaddr_in destAddr;
	struct hostent *he;

	packetHeader p;
	memcpy(&p, packet, sizeof(packetHeader));
	
	routingLock.lock();
	bool res = routingTable.find(p.destNodeID) == routingTable.end();
	routingLock.unlock();
	if(res)
	{
		cout<<"Couldn't find destination in routing table (wat)"<<endl;
	}
	else
	{
		int destNodeSend = routingTable[p.destNodeID].intermediateNode;
		if(routingTable[p.destNodeID].intermediateNode == nodeID)
		{
			destNodeSend = p.destNodeID;
		}

		he = gethostbyname(configTable[destNodeSend].hostName.c_str());
		if(he == NULL)
		{
			cerr<<"Error with gethostbyname in send"<<endl;
			cerr<<h_errno<<endl;
			exit(1);
		}
	
		destAddr.sin_family = AF_INET;
		destAddr.sin_port = htons(configTable[destNodeSend].dataPort); //dest ctrl port
		memcpy(&destAddr.sin_addr, he->h_addr, he->h_length); //dest ip

		int result = sendto(dataSocket, packet, PACKET_SIZE, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
		if(result == -1)
		{
			cerr<<"Error sending data packet"<<endl;
			cout<<strerror(errno)<<endl;
			exit(1);
		}
	}
}



