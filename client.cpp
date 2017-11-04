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

int main(int argc, char *argv[]){

	
	if(argc < 2){
		cout<<"usage : "<<argv[0]<<" config.txt"<<endl;
		exit(1);
	}
	

	if(argv[2] == "generate-packet"){
		if(generatePacket(atoi(argv[3]),atoi(argv[4])) == -1){
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
		if(initialize(argv[2]) == -1){
			cout << "Initialization Failed" << endl;
			exit(1)
		}
	}

}

int generatePacket(int sender,int reciever){
	
}

int createLink(int node1, int node2){
	
}

int removeLink(int node1, int node2){

}

int initialize(string file){
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
		//need to handle some random number of neighbors after
		int n;
		while(in>>n){
			neighbors.push_back(n);
		}
	}
	inFile.close();
}


