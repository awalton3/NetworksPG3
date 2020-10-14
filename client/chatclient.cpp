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
#include <map>
#include "pg3lib.h"
#define MAX_SIZE 4096

using namespace std;

/* Globals */ 
int sockfd;
typedef struct {
	map<int, char*> active_sockets_map;
} active_sockets_struct; 


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

void send_str(int sockfd, string command) { 
	char commandF[BUFSIZ]; 
	strcpy(commandF, command.c_str());  
	if (send(sockfd, commandF, strlen(commandF) + 1, 0) == -1) {
        perror("Error sending command to server."); 
    }
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
    while (1) {  //FIXME: change so that there is only one recv in here, and it parses out D vs. C for data and command messages
        // Receive acknowledgement first 
        int ack;
        if (recv(sockfd, &ack, sizeof(ack), 0) == -1) {
            perror("Receive acknowledgement error\n");
        }
        cout << "HELLO??" << endl;
        ack = ntohl(ack);
        // Receive BM message
        char msg[MAX_SIZE];
        if (recv(sockfd, &msg, sizeof(msg), 0) == -1) {
            perror("Receive message error \n");
        }
        cout << "Message received: " << msg;
    } 

    return 0;
}

/* Client functionality */
void broadcast(int sockfd){
    // Send broadcast command to server
    send_str(sockfd, "BM");
    cout << "Sent BM" << endl;
    // Receive acknowledgement
    /*int ack;
    cout << "trying to receive?" << endl;
    if (recv(sockfd, &ack, sizeof(ack), 0) == -1) {  
        perror("Receive acknowledgement error\n");
    }
    cout << "HELLO??" << endl;
    ack = ntohl(ack);
    
    cout << "Received ack num: " << ack << endl;*/
    int ack = 0;  // TODO: how to get the ack number here if it is received in other thread?
    if (ack == 1) {
        // Send message to the user
	    char msg[BUFSIZ];
        cout << ">Enter the public message: " << endl;
        scanf("%s", msg);

        if (send(sockfd, msg, strlen(msg) + 1, 0) == -1){
            perror("Error sending message\n");
        }
    }
}

int print_active_users(int sockfd) {

	// Receive number of users
	int n_users; 
	if(recv(sockfd, &n_users, sizeof(n_users), 0) == -1) {
		perror("Error receiving number of users"); 
		return -1; 
	}
	n_users = ntohl(n_users); 

	cout << "Number of users: " << n_users << endl; 

	// Send ack to server 
	int ack = (n_users >= 0 ? 1 : -1); 
	ack = htonl(ack);
	if(send(sockfd, &ack, sizeof(ack), 0) == -1) {
		perror("Error sending ack to server\n"); 
		return -1; 
	}

	cout << "Sent ack" << endl; 

	cout << "Peers online:\n"; 

	// Receive active users from server 
	for (int i = 0; i < n_users; ) {
		char username[MAX_SIZE]; 
		if (recv(sockfd, &username, sizeof(username), 0) == -1) {
			perror("Error receiving username\n");
			return -1; 
		}
		cout << username << endl; 
	}

	return n_users; 
}

void private_message(int sockfd) {

	//TODO divide up and put in correct threads

	// Send operation to server 
	send_str(sockfd, "PM"); // TODO error check
	
	// Get active users from server 
	int n_users = print_active_users(sockfd); 
	
	/*active_sockets_struct *active_users = new active_sockets_struct(); 
	map<int, char*> buffer; 
	active_users->active_sockets_map = buffer; 

	if(recv(sockfd, (active_sockets_struct*) &active_users, sizeof(active_users), 0) == -1) {
		perror("Error receiving active users from server\n"); 
		return; 
	}

	cout << "Received active user list: " << active_users << endl; 

	for (auto const& user : active_users->active_sockets_map) {
		cout << user.first << ": " << user.second; 
	} */ 

	// Prompt user to enter target user
	if (n_users > 0) {

		char target[MAX_SIZE];
   		cout << ">Peer to message: ";	
		cin >> target; 

		// Sends username to server
		if(send(sockfd, &target, sizeof(target), 0) == -1){
			perror("Error sending recipient to server"); 
			return; 
		}

		cout << "Sent target user to server\n"; 
	}


	// Get pubkey from server 
	
	// Get message from the user
}

void exit_client(int sockfd) {
    // Send EXIT operation to server
    send_str(sockfd, "EX");

    // Close socket and leave
    close(sockfd);
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
	cout << "Received pubKey: " << pubkey << endl; 

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
