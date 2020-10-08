CFLAGS=-g -c -std=c++11
LDFLAGS=-g -lcrypto -lz
 
all: chatserver

myftpd: chatserver.o
	g++ $(LDFLAGS) -o chatserver chatserver.o

myftpd.o: chatserver.cpp
	g++ $(CFLAGS) chatserver.cpp

clean: 
	rm -f *.o chatserver
