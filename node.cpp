#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <thread>  

//routing table 
typedef struct{
	int destinationNode;
	int lastTraveledNode;
	int distance;
}routeStruct;

using namespace std;

int nodeid, ctrlPort, dataPort;
string hostName;

map<int,routeStruct> routingTable;

int main(int argc, char *argv[])
{
	Node thisNode;
	int thisNodeID = argv[2];
	
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
		
		if(nodeID == thisNodeID)
		{
			int n;
			while(in>>n)
			{
				routeStruct neighbor = {distance = 1; lastTraveledNode = nodeID; destinationNode = n;}
				routingTable.insert(pair <int, routeStruct> (n, neighbor));
			}
		}
	}
	inFile.close();
	
	
	//create control thread here?
	//have it always wait in recieveDistanceVector
	while(1)
	{
		sendDistanceVector();
		receiveDistanceVector();
		sleep(5);
	}
	
	
	
}

//i guess we would call this from the outside function?
void sendDistanceVector(){
	char* outboundHostName;
	struct addrinfo hints, *res;
	struct sockaddr_storage addr;
	socklen_t tolen;
	char ipstr[INET6_ADDRSTRLEN];
	int sockfd, byteCount;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(NULL, itoa(ctrlPort), &hints, &res);
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);//so this is this nodes socket i think
	bind(sockfd, res->ai_addr, res->ai_addrlen);
	
	
	char buf[512];
	tolen = sizeof addr;
	byteCount = recvfrom(sockfd,buf,sizeof buf, 0,&addr,&tolen);
	if(byteCount == -1){
		cerr<<"Error recieving packet"<<endl;
	}
}

//idk if this is necesary
//before we can recieve we have to connect to all neighbor nodes
//have to make the structs the hold the addresses of the senders to know who send the distance vector
void receiveDistanceVector(){
	char* inboundHostName;
	struct addrinfo hints, *res;
	struct sockaddr_storage addr;
	socklen_t fromlen;
	char ipstr[INET6_ADDRSTRLEN];
	int sockfd, byteCount;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(NULL, itoa(ctrlPort), &hints, &res);
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);//so this is this nodes socket i think
	bind(sockfd, res->ai_addr, res->ai_addrlen);

	char buf[512];
	fromlen = sizeof addr;
	byteCount = recvfrom(sockfd,buf,sizeof buf, 0,&addr,&fromlen);//unsure of what to do with sockfd
	if(byteCount == -1){
		cerr<<"Error recieving packet"<<endl;
	}
	//determine which neighbor this shit came from with getaddr info or whatever that shit was
	//parse buffer for the distance vector

}
