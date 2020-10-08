/* Team members: Crystal Colon, Molly Zachlin, Auna Walton 
 * NetIDs: ccolon2, mzachlin, awalton3
 * chatserver.cpp - The server side of the TCP chat application  
 *   
*/ 
#include <iostream>
#include <string>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
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
#include "pg3lib.h"

#define MAX_SIZE 4096
#define MAX_PENDING 5

using namespace std;

/* Authenticate user */ 
char* authenticate(char* username) {

	// Prepare to authenticate 
	FILE *user_file = fopen("users.txt", "r"); 
	if (!user_file) {
		perror("Could not open users file\n"); 
		exit(-1); 
	} 

	// Check if user exists 
	char line[MAX_SIZE];
	char* file_user; 
	char* password; 
	while(fgets(line, MAX_SIZE, user_file)) {
		file_user = strtok(line, ","); 
		if(strcmp(username, file_user) == 0) {
            password = strtok(NULL, ",");
            fclose(user_file);
			return password; // username exists 
		}
	}
    
    fclose(user_file);
	return NULL; // username does not exist	
}

/* Login user */
void login(char* username, char* password, char* filePassword, int new_sockfd) {
    int incorrect;
    cout << "FP: " << filePassword << endl;
    if (strcmp(password, filePassword) != 0) 
        cout << "no match." << endl;
    while (strcmp(password, filePassword) != 0) { // Passwords do not match.

        cout << "password: " << password << "fp: " << filePassword << endl;

        incorrect = htonl(1);
        if (send(new_sockfd, &incorrect, sizeof(incorrect), 0) == -1) {
            perror("Error sending password code to client");	
	    }
        // Receive new password attempt from client
        char encrypted_password[MAX_SIZE]; 
	    if (recv(new_sockfd, &encrypted_password, sizeof(encrypted_password), 0) == -1) {
		    perror("Error receiving encrypted password from client\n"); 
	    }
	    password = decrypt(encrypted_password); 
    }

    incorrect = htonl(0);  // Passwords match; send code
    if (send(new_sockfd, &incorrect, sizeof(incorrect), 0) == -1) {
        perror("Error sending password code to client");	
	}    
}

/* Add new user */
void addUser(char* username, char* password) {
    FILE* user_file = fopen("users.txt", "a");
    int size = strlen(username) + strlen(password) + 2;
    char userLine[size];
    // Append username,password to users.txt
    snprintf(userLine, size, "%s,%s", username, password);
    fwrite(userLine, size, 1, user_file);
    fwrite("\n", sizeof(char), 1, user_file);
    fclose(user_file);
}

/* Handle connection for each client */ 
void *connection_handler(void *socket_desc)
{
    int new_sockfd = *(int*) socket_desc;
    char user[MAX_SIZE];
	char* pubKey = getPubKey(); 

	// Send username to server
    if (recv(new_sockfd, &user, sizeof(user), 0) ==-1){
        perror("Received username error\n");
    }
    cout << "Received username : " << user << endl;

	// Send public key to client
	if (send(new_sockfd, pubKey, strlen(pubKey) + 1, 0) == -1) {
        perror("Error sending public key to client");	
	}

	// Check if user is authenticated
    char* filePassword = authenticate(user);
    int user_exists;
    if (filePassword) 
	    user_exists = htonl(1);
    else 
        user_exists = htonl(0);

    cout << "LOOK fp: " << filePassword << endl;
    if (send(new_sockfd, &user_exists, sizeof(user_exists), 0) == -1) {
        perror("Error sending user authentication to client");
    } 

	// Receive password from client 
	char encrypted_password[MAX_SIZE]; 
	if (recv(new_sockfd, &encrypted_password, sizeof(encrypted_password), 0) == -1) {
		perror("Error receiving encrypted password from client\n"); 
	}
	char* password = decrypt(encrypted_password); 
	cout << "Received password: " << password << endl; 

    // Login or register user 
    if (user_exists) {
        login(user, password, filePassword, new_sockfd);
    } 
    else {
        addUser(user, password);
    }
             
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
