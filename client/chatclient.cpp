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

/* Globals */ 
int sockfd;
// Thread condition variable and lock
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 
int i = 1;
bool ready = false;
char server_msg[MAX_SIZE] = {0};  // stores messages from server such as the pubkey
char last_console[MAX_SIZE];
char pm_ack[MAX_SIZE];

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

/* Threading */
void *handle_messages(void*) {
    while (1) {  
       	
        // Receive message from server
        char msg[MAX_SIZE];
		bzero(msg, MAX_SIZE); 
        if (recv(sockfd, &msg, sizeof(msg), 0) == -1) {
            perror("Receive message error \n");
        } 
        
        char decoded_msg[MAX_SIZE]; 

		// Handle data message 
       	if (msg[0] == 'D') { 
            char* real_msg = msg;
            real_msg++;
    		char* decrypted_msg = decrypt(real_msg);

            cout << "**** Incoming Private Message ****: " << decrypted_msg << endl;
            cout << last_console;
        }
		// Handle command message 
        else if (msg[0] == 'C') { 
	        char decoded_msg[MAX_SIZE]; 
	        strcpy(decoded_msg, msg + 1); 
            strcpy(server_msg, decoded_msg);
            strcpy(pm_ack, decoded_msg);

            // Acquire the lock 
		    pthread_mutex_lock(&lock); 
			ready = true;
        }

        // Handle broadcast message
        else if(msg[0] == 'B'){
            strcpy(decoded_msg, msg + 1); 
            cout << "**** Incoming Public Message ****: " << decoded_msg << endl;
            cout << last_console;
        }
        else if(msg[0] == 'Z'){
            //Ack received
            strcpy(decoded_msg, msg + 1); 
        }
        else if(msg[0] == 'L'){
            //Confirmation received
            strcpy(decoded_msg, msg + 1); 
        }
        // Handle users list
        else if (msg[0] == 'U') { 
            strcpy(decoded_msg, msg + 1); 
            cout << decoded_msg << endl;
            // Acquire the lock 
		    pthread_mutex_lock(&lock);
            ready = true;
        }
        // Handle PM acknowledgement
        /*else if (msg[0] == 'A') {
            char* real_msg = msg;
            real_msg++;
            strcpy(pm_ack, real_msg);
            // Acquire the lock 
		    pthread_mutex_lock(&lock);
            ready = true;
        } */
		// Handle invalid message 
        else {
            cout << "msg received " << msg<< endl;
            cout << "Received corrupted message from server." << endl;
        }

		if (ready) {
            // Wake up sleeping threads 
		    pthread_cond_signal(&cond); 
	     	// Release lock 
		    pthread_mutex_unlock(&lock); 
        }
    } 

    return 0;
}

/* Client functionality */
void broadcast(int sockfd){	
    
    // Send operation to server 
	send_str(sockfd, "BM");
   
	char message[MAX_SIZE]; 
  	cout << ">Enter the message to broadcast:"; 
    strcpy(last_console, ">Enter message to broadcast:");     
    fgets(message,MAX_SIZE,stdin);
     
	// Send message to server 
	if(!send_str(sockfd, message, "Error sending private message to server"))
		return;

	//ready = false;
}

void private_message(int sockfd) {

	// Send operation to server 
	send_str(sockfd, "PM"); 

	// Prepare output for listing of active users 
    cout << "Peers online:" << endl;
		
	// Acquire lock
	pthread_mutex_lock(&lock);

    // Sleep until active users are given
	while (!ready) {
        pthread_cond_wait(&cond, &lock);
    }
   
    // Prompt user to enter target user
	char target[MAX_SIZE];
   	cout << ">Peer to message: ";
    strcpy(last_console, ">Peer to message: ");    
	fgets(target, MAX_SIZE, stdin);
	
   	int len=strlen(target); 
	if(target[len-1]=='\n')
	    target[len-1]='\0';	

	// Sends username to server
	if(send(sockfd, target, strlen(target) + 1, 0) == -1){
		perror("Error sending recipient to server"); 
		return; 
	}

	// Release lock
    ready = false; 	
	pthread_mutex_unlock(&lock);

    // Acquire lock
	pthread_mutex_lock(&lock);

	// Sleep until pubkey is received  
    while (!ready) {
        pthread_cond_wait(&cond, &lock);
	}    
    
	// Get message from the user
	char message[MAX_SIZE]; 
	cout << ">Enter the private message:"; 
    strcpy(last_console, ">Enter the private message:");  

	char line[MAX_SIZE]; 

    fgets(message, MAX_SIZE, stdin);

	fflush(stdin); 

	len=strlen(message);
	if(message[len-1]=='\n')
		message[len-1]='\0';
    
	// Encrypt message
    /*if (server_msg[0] != 0) {
        cout << "User does not exist!" << endl;
        cout << last_console;
        return;
    } */
	char* encrypt_msg = encrypt(message, server_msg);  
    

    // Send message to server 
	if(!send_str(sockfd, encrypt_msg, "Error sending private message to server"))
		return;

	// Release lock
    ready = false;
	pthread_mutex_unlock(&lock);
    
    // Acquire lock
	pthread_mutex_lock(&lock);
	
    // Sleep until confirmation is received
    while (!ready) {
        pthread_cond_wait(&cond, &lock);
    }
    
    if (strcmp(pm_ack, "0") == 0) {
       cout << "User does not exist!" << endl;
    }
    else {
        cout << "Private message sent." << endl;
    }

    // Release lock
    ready = false;
	pthread_mutex_unlock(&lock);

}

void exit_client(int sockfd, int thread_id) {
    // Send EXIT operation to server
    send_str(sockfd, "EX");

    // Close socket and leave
    close(sockfd);

    // Join threads
    pthread_join(thread_id, NULL);
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

	switch (isUser) {
		case 0: cout << "Creating new user\n"; break; 
		case 1: cout << "Existing user\n"; break; 	
		default: exit(-1); 
	}

    // Get user password
	char password[MAX_SIZE]; 
	cout << "Enter password:"; 
	//scanf("%s", password);   
	fgets(password, MAX_SIZE, stdin); 

	int len=strlen(password);
	if(password[len-1]=='\n')
		password[len-1]='\0';

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
        	fgets(password, MAX_SIZE, stdin); 

			int len=strlen(password);
			if(password[len-1]=='\n')
				password[len-1]='\0';

            // Send new password back to server
            char* encrypted_password = encrypt(password, pubkey); 
	        if (send(sockfd, encrypted_password, strlen(encrypted_password) + 1, 0) == -1) {
                perror("Error sending password to server");
	        }
        }
    }

	// Receive ack from server
	int ack = 0;  
	if (recv(sockfd, &ack, sizeof(ack), 0) == -1) {
	    perror("Error receiving ack"); 	
	}      
	ack = ntohl(ack); 

	if (ack != 1) {
		perror("Acknowledgement failed\n"); 
		return 1; 
	}

	// Send client pubkey to server
	char* client_pubkey = getPubKey(); 
	if(send(sockfd, client_pubkey, strlen(client_pubkey) + 1, 0) == -1) {
		perror("Error sending pubkey to server"); 
		return 1; 
	}

    cout << "Connection established." << endl; 

    pthread_t thread;
    int rc = pthread_create(&thread, NULL, handle_messages, NULL);

	char op[MAX_SIZE]; 
    while(1){
        if(rc) {
            cout << "Error: unable to create thread" << endl;
            exit(-1);
        }
        cout << ">Please enter a command (BM: Broadcast Messaging, PM: Private Messaging, EX: Exit)" << endl;
        cout << "> ";  
        strcpy(last_console, ">Please enter a command (BM: Broadcast Messaging, PM: Private Messaging, EX: Exit)\n> ");     
		fgets(op, MAX_SIZE, stdin);

		int len=strlen(op);
		if(op[len-1]=='\n')
			op[len-1]='\0';

        if (strcmp("PM", op) == 0) {
            private_message(sockfd);
        }
        else if (strcmp("BM", op) == 0) {
            broadcast(sockfd);

        }
        else if (strcmp("EX", op) == 0) {
            exit_client(sockfd, rc);
        }
        else{
            cout << "Invalid entry" << endl;
        }
    }
 
	return 0; 
}
