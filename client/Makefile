CFLAGS=-g -c -std=c++11
LDFLAGS=-g -lcrypto -lz -lpthread
 
all: chatclient

chatclient: chatclient.o
	g++ $(LDFLAGS) -o chatclient chatclient.o

chatclient.o: chatclient.cpp
	g++ $(CFLAGS) chatclient.cpp

clean: 
	rm -f *.o chatclient
