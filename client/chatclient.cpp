/* Team members: Crystal Colon, Molly Zachlin, Auna Walton 
 * NetIDs: ccolon2, mzachlin, awalton3
 * chatclient.cpp - The client side of the TCP chat application  
 *   
*/ 

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#define MAX_SIZE 4096
using namespace std;

void error(int code) {
    switch (code) {
        case 1:
            cout << "Not enough input arguments!" << endl;
        break; 
        default:
            cout << "An unknown error occurred." << endl;
        break;
    }
}
void *handle_messages(void*){
    return 0;
}
void private_message(){
    cout << "Private message entered" << endl;
}
void broadcast(){
    cout << "Broadcast message entered" << endl;
}

int main(int argc, char** argv) {
    /* Parse command line arguments */
    if (argc < 4) { // Not enough input args
        error(1); 
        return 1;
    }

    const char* host = argv[1];
    int port = stoi(argv[2]);
    char* user = argv[3];
    struct stat statbuf;
    string op;
       
    /* Translate host name into peer's IP address */
    struct hostent* hostIP = gethostbyname(host);
    if (!hostIP) {
        cout << "Error: Unknown host." << endl;
        return 1;
    }

    /* Create address data structure */
    struct sockaddr_in sock;
    // Clear memory location
    bzero((char*) &sock, sizeof(sock));
    // Specify IPv4
    sock.sin_family = AF_INET;
    // Copy host address into sockaddr
    bcopy(hostIP->h_addr, (char*) &sock.sin_addr, hostIP->h_length);
    // Convert from host to network byte order
    sock.sin_port = htons(port);

    /* Create the socket */
    int sockfd;
    //socklen_t len = sizeof(sock);
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error creating socket.");
        return 1;
    }

	cout << "Connecting to " << host << " on port " << port << endl; 

    /* Connect */
    if (connect(sockfd, (struct sockaddr*)&sock, sizeof(sock)) < 0) {
        perror("Error connecting.");
        return 1;
    }
    
    cout << "Connection established" << endl;

    if(send(sockfd,user,strlen(user) + 1 ,0) == -1 ){
        perror("Error in sending the username\n");

    }
    

    pthread_t thread;
    int rc = pthread_create(&thread,NULL,handle_messages,NULL);

    while(1){
        if(rc){
            cout << "Error: unable to create thread" << endl;
            exit(-1);
        }
        std::cin >> op;
        if(op=="PM"){
            private_message();
        }
        else if(op=="BM"){
            broadcast();
        }
        else{
            cout << "Invalid entry" << endl;
        }
    }
    
    


	return 0; 
}
