all: node

# compile node
node: node.cpp
	g++ -std=c++0x -g -o node node.o

clean:
	rm -f *.o node
	rm -f *.exe
