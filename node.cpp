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

class node
{
	public:
		
	private: //maybe just make them all public?//sounds good, i dont care about design patterns ;)
		int nodeid, ctrlPort, dataPort;
		string hostName;
		vector<int> neighbors;
		map<int,node> routingTable;
}//



int main(int argc, char *argv[])
{
	int sockfd, port;
	char *hostname;
	
	
	if(argc != 2)
	{
		cout<<"usage : "<<argv[0]<<" config.txt"<<endl;
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
		//instantiate a class first?
		istringstream in(line);
		in>>nodeID>>hostName>>ctrlPort>>dataPort;
		//need to handle some random number of neighbors after
		int n;
		while(in>>n){
			neighbors.push_back(n);
		}
	}
	inFile.close();
	
	
	/*sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0)
	{
		perror("Error opening socket");
	}*/
	
	//create control thread here?
	//have it always wait in recieveDistanceVector
	
	
	
}

//i guess we would call this from the outside function?
void sendDistanceVector(){

}

//idk if this is necesary
//before we can recieve we have to connect to all neighbor nodes
//have to make the structs the hold the addresses of the senders to know who send the distance vector
void recieveDistanceVector(){
	char* inboundHostName;
	struct addrinfo hints, *res;
	struct sockaddr_storage addr;
	socklen_t fromlen;
	char ipstr[INET6_ADDRSTRLEN];

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
	if(byteCOunt == -1){
		cerr<<"Error recieving packet"<<endl;
	}
	//determine which neighbor this shit came from with getaddr info or whatever that shit was
	//parse buffer for the distance vector

}