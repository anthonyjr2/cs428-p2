all: node client

node: node.o
	g++ node.o -o node

node.o: node.cpp
	g++ -g -c -std=c++1z node.cpp -o node.o
	
client: client.o
	g++ client.o -o client
	
client.o: client.cpp
	g++ -g -c -std=c++1z client.cpp -o node.o

clean:
	rm -f *.o node
	rm -f *.o client
	rm -f *.exe
