# compile client and server
all: node

# compile client only
node: node.cpp
	g++ -g -o node node.o

clean:
	rm -f *.o node
	rm -f *.exe
