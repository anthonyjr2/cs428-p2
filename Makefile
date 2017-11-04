all: node

node: node.o
	g++ node.o -o node

node.o: node.cpp
	g++ -g -c -std=c++1z node.cpp -o node.o

clean:
	rm -f *.o node
	rm -f *.exe
