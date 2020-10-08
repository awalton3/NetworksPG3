CFLAGS=-g -c -std=c++11
LDFLAGS=-g -lcrypto -lz
 
all: chatclient

myftp: chatclient.o
	g++ $(LDFLAGS) -o chatclient chatclient.o

myftp.o: chatclient.cpp
	g++ $(CFLAGS) chatclient.cpp

clean: 
	rm -f *.o chatclient
