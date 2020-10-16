/* Team members: Crystal Colon, Molly Zachlin, Auna Walton 
 * NetIDs: ccolon2, mzachlin, awalton3
 * chatclient.cpp - The client side of the TCP chat application  
 *   
*/ 

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <map>
#include "pg3lib.h"
#define MAX_SIZE 4096

using namespace std;

//FIXME:improve error checking and returns after perrors

/* Globals */ 
int sockfd;
// Thread condition variable and lock
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 
bool ready = false;
char server_msg[MAX_SIZE] = {0};  // stores messages from server such as the pubkey
char last_console[MAX_SIZE];

/* Helper Functions */ 
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

bool send_str(int sockfd, string command) { 
	char commandF[BUFSIZ]; 
	strcpy(commandF, command.c_str());  
	if (send(sockfd, commandF, strlen(commandF) + 1, 0) == -1) {
        perror("Error sending command to server."); 
		return false;     
	}
	return true; 
}

bool send_str(int sockfd, char* msg, string error) {
	if (send(sockfd, msg, strlen(msg) + 1, 0) == -1) {
        cout << error << endl; 
		return false;     
	}
	return true; 
}

void send_int(int sockfd, int command) {
	command  = htonl(command); 
	if (send(sockfd, &command, sizeof(command), 0) == -1) {
        perror("Error sending command to server."); 
        return;
    }
}

/*char* decode_msg(char* msg) {
	char decoded_msg[MAX_SIZE]; 
	strcpy(decoded_msg, msg + 1); 
	return decoded_msg; 
}*/


/* Threading */
void *handle_messages(void*) {
    while (1) {  

        //cout << "start the whileee" << endl;

        // Receive message from server
        char msg[MAX_SIZE];
        if (recv(sockfd, &msg, sizeof(msg), 0) == -1) {
            perror("Receive message error \n");
        }
	    
        //cout << "from server: " << msg << "###" << endl;

        printf("receivied message: %s\n",msg);
        
        char decoded_msg[MAX_SIZE]; 
		// Handle data message 
       	if (msg[0] == 'D') {   
            strcpy(decoded_msg, msg + 1); 
            cout << "**** Incoming Private Message ****: " << decoded_msg << endl;
            // Acquire the lock 
		    pthread_mutex_lock(&lock);
            ready = true;
            //cout << "ready is now true" << endl;
            cout << last_console << endl;
        }
		// Handle command message 
        else if (msg[0] == 'C') { 
	        char decoded_msg[MAX_SIZE]; 
	        strcpy(decoded_msg, msg + 1); 
            strcpy(server_msg, decoded_msg);
            // Acquire the lock 
		    pthread_mutex_lock(&lock);  //FIXME: duplicate; could move after if/else?
            ready = true;
        }
        // Handle users list
        else if (msg[0] == 'U') { 
            //cout << "receiving users" << endl;
            strcpy(decoded_msg, msg + 1); 
            cout << decoded_msg << endl;
            // Acquire the lock 
		    pthread_mutex_lock(&lock);
            //cout << "I ACQUIRED LOCK>>" << endl;
            ready = true;
        }
		// Handle invalid message 
        else {
            cout << "Received corrupted message from server." << endl;
        }

		if (ready) {
            // Wake up sleeping threads 
		    pthread_cond_signal(&cond); 
        
            //cout << "HI HI HI HI HI I AM AWAKE" << endl;
		
            // Release lock 
		    pthread_mutex_unlock(&lock); 
        }
        
        //cout << "this is an example" << endl;
    } 

    return 0;
}

/* Client functionality */
void broadcast(int sockfd){	// Send operation to server 
	send_str(sockfd, "BM"); 



	char message[MAX_SIZE]; 
	cout << ">Enter the message to broadcast:"; 
        strcpy(last_console, ">Enter message to broadcast:");  
        cin >> message;
        
        printf("message: %s\n", message);
	// Send message to server 
	if(!send_str(sockfd, message, "Error sending private message to server"))
		return;

	    ready = false;
}

void private_message(int sockfd) {

	// Send operation to server 
	send_str(sockfd, "PM"); 

	// Prepare output for listing of active users 
    cout << "Peers online:" << endl;
		
	// Acquire lock
	pthread_mutex_lock(&lock);

    //cout << "got the lock" << endl;
        
    // Sleep until active users are given 
	while (!ready) {
        pthread_cond_wait(&cond, &lock);
    }
   
    //cout << "done waiting for active users " << endl;

    // Prompt user to enter target user
	char target[MAX_SIZE];
   	cout << ">Peer to message: ";
    strcpy(last_console, ">Peer to message: ");    
	cin >> target;
    cin.get(); 

	// Sends username to server
	if(send(sockfd, &target, sizeof(target), 0) == -1){
		perror("Error sending recipient to server"); 
		return; 
	}

	// Release lock
    ready = false;  //TODO: comment out
	pthread_mutex_unlock(&lock);

    // Acquire lock
	pthread_mutex_lock(&lock);

	// Sleep until pubkey is received  
    while (!ready) {
        pthread_cond_wait(&cond, &lock);
	}    
    
	//cout << "Server_msg: " << server_msg << endl;

	// Get message from the user
    //string cpp_msg;
	char message[MAX_SIZE]; 
	cout << ">Enter the private message:"; 
    strcpy(last_console, ">Enter the private message:");  
    fgets(message, MAX_SIZE, stdin);
    
	// Encrypt message 
	
	char* encrypt_msg = encrypt(message, server_msg);  

	// Send message to server 
	if(!send_str(sockfd, message, "Error sending private message to server"))
		return;

	// Release lock
    ready = false;
	pthread_mutex_unlock(&lock);

}

void exit_client(int sockfd) {
    // Send EXIT operation to server
    send_str(sockfd, "EX");

    // Close socket and leave
    close(sockfd);

    //TODO: add pthread_join here to join threads
    exit(1); 
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
    //int sockfd;
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

	// Send username to server 
    if(send(sockfd, user, strlen(user) + 1 , 0) == -1){
        perror("Error in sending the username\n");
    }

	// Get pubkey from server 
	char pubkey[MAX_SIZE]; 
	if (recv(sockfd, &pubkey, sizeof(pubkey), 0) == -1) {
		perror("Error receiving pubkey from server\n"); 
	}

	char* pubKey = pubkey; 


	// Check if user is authenticated 
	int isUser = 0; 
	if (recv(sockfd, &isUser, sizeof(isUser), 0) == -1) {
		perror("Error receiving user authentication"); 	
	}
	isUser = ntohl(isUser); 
	cout << "Received auth status : " << isUser << endl;

	switch (isUser) {
		case 0: cout << "Creating new user\n"; break; 
		case 1: cout << "Existing user\n"; break; 	
		default: exit(-1); 
	}

    // Get user password
	char password[MAX_SIZE]; 
	cout << "Enter password:"; 
	scanf("%s", password);   

	// Send password to server 
	char* encrypted_password = encrypt(password, pubkey); 
	if(send(sockfd, encrypted_password, strlen(encrypted_password) + 1, 0) == -1) {
        perror("Error sending password to server");
	}

    if (isUser) { 
        // Verify password
        int incorrect = 1;
        while (incorrect) {
            if (recv(sockfd, &incorrect, sizeof(incorrect), 0) == -1) {
		        perror("Error receiving password status"); 	
	        }      
            if (!incorrect) // Password is correct!
                break;  

            cout << "Incorrect password.  Please enter again:";
            scanf("%s", password);
            // Send new password back to server
            char* encrypted_password = encrypt(password, pubkey); 
	        if (send(sockfd, encrypted_password, strlen(encrypted_password) + 1, 0) == -1) {
                perror("Error sending password to server");
	        }
        }
    }

	//TODO should the stuff above be a part of the main thread ????
    cout << "Connection established." << endl;  //FIXME: is this the right spot for this line?

    pthread_t thread;
    int rc = pthread_create(&thread, NULL, handle_messages, NULL);
    while(1){
        if(rc) {
            cout << "Error: unable to create thread" << endl;
            exit(-1);
        }
        cout << ">Please enter a command (BM: Broadcast Messaging, PM: Private Messaging, EX: Exit)" << endl;
        cout << "> ";  
        strcpy(last_console, ">Please enter a command (BM: Broadcast Messaging, PM: Private Messaging, EX: Exit)\n> ");     
        cin >> op;
        if (op == "PM") {
            private_message(sockfd);
        }
        else if (op == "BM") {
            broadcast(sockfd);

        }
        else if (op == "EX") {
            exit_client(sockfd);
        }
        else{
            cout << "Invalid entry" << endl;
        }
    }
 
	return 0; 
}
