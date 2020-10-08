/* Team members: Crystal Colon, Molly Zachlin, Auna Walton 
 * NetIDs: ccolon2, mzachlin, awalton3
 * chatserver.cpp - The server side of the TCP chat application  
 *   
*/ 
#include <iostream>
#include <string>

#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#define MAX_SIZE 4096
#define MAX_PENDING 5

using namespace std;

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    int new_sockfd = *(int*) socket_desc;
    char user[MAX_SIZE];
    if(recv(new_sockfd,&user,sizeof(user),0) ==-1){
        perror("Received username error\n");
    }
    cout << "Received username : " << user << endl;
             
    return 0;
} 

void error(int code) {
    switch (code) {
        case 1:
            cout << "Not enough arguments" << endl;
        break;
        default:
            cout << "An unknown error occurred" << endl;
        break;
    }
}


int main(int argc, char** argv) {

    /* Parse command line arguments */
    if (argc < 2) {
        error(1);
        return 1;
    }

    int port = stoi(argv[1]);

    /* Create address data structure */
    struct sockaddr_in sock;
    // Clear memory location
    bzero((char*) &sock, sizeof(sock));
    // Specify IPv4
    sock.sin_family = AF_INET;
    // Use the default IP address of the server
    sock.sin_addr.s_addr = INADDR_ANY;
    // Convert from host to network byte order
    sock.sin_port = htons(port);

    /* Set up socket */
    int sockfd;
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed.");
        return 1;
    }

    /* Set socket options */
    int opt = 1;
    if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(char*)&opt, sizeof(int))) < 0) {
        perror("Setting socket options failed.");
        return 1;
    }

    /* Bind */ 
    if ((bind(sockfd, (struct sockaddr*) &sock, sizeof(sock))) < 0) {
        perror("Bind failed.");
        return 1;
    }

    /* Listen */
    if ((listen(sockfd, MAX_PENDING)) < 0) {
        perror("Listen failed.");
        return 1; 
    } 

    /* Wait for incoming connections */
    int new_sockfd;
    struct sockaddr_in client_sock;
    socklen_t len = sizeof(client_sock);
    pthread_t thread_id;

    while (1) {

        cout << "Waiting for connections on port " << port << endl;
        if ((new_sockfd = accept(sockfd, (struct sockaddr*) &client_sock, &len)) < 0) {
			cout << "exiting " << endl; 
            perror("Accept failed.");
			cout << "exiting " << endl; 
            return 1;
        }

        cout << "Connection established." << endl;

        if(pthread_create(&thread_id, NULL, connection_handler, (void*) &new_sockfd) < 0) {
            perror("could not create thread");
            return 1;
        }
 
    }
	return 0;
}
