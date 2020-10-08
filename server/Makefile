CFLAGS=-g -c -std=c++11
LDFLAGS=-g -lcrypto -lz -lpthread
 
all: chatserver

chatserver: chatserver.o
	g++ $(LDFLAGS) -o chatserver chatserver.o

chatserver.o: pg3lib.h chatserver.cpp
	g++ $(CFLAGS) chatserver.cpp

clean: 
	rm -f *.o chatserver
