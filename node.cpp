#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>

using namespace std;

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
	
	
	/*sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0)
	{
		perror("Error opening socket");
	}*/
	
	
	
	
	
}