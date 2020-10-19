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
#include <map>
#include "pg3lib.h"

#define MAX_SIZE 4096
#define MAX_PENDING 5

using namespace std;

/* Global variables */
map<int, char*> ACTIVE_SOCKETS;  // Keeps track of {active socket, username}
map<int, char*> CLIENT_KEYS; // Keeps track of {active socket, client pubkey}
bool ack_sent = false;

typedef struct {
	map<int, char*> active_sockets_map;
} active_sockets_struct; 


/* Server functionality methods */ 

/* Broadcast message to all clients */
/* Helper Functions */

/* Sends either a data or command message to the client
 * type - should be D or C
 * return true if the msg was sent successfully 
 */

/* Searches for user in active users map structure
 * returns the sockfd if the user is found 
 * otherwise, returns -1 
 */
//}
bool send_msg(char type, int sockfd, char* msg, string error) {

    if (type != 'D' && type != 'C' && type != 'U' && type !='B' && type!='Z' && type!='L') 
        return false;

    // Add type to front of message
    char new_msg[MAX_SIZE];
    sprintf(new_msg, "%c%s", type, msg);
    
    //new_msg[strlen(new_msg)] = '\0';
    
    cout << "new msg: *" << new_msg <<"*"<< endl;

    // Send message to client
    if (send(sockfd, new_msg, strlen(new_msg) + 1, 0) == -1) {
		perror("system error"); 
        cout << error << endl;
        return false;
    } 
    return true;
}


void send_active_users(int sockfd) {
	char output[MAX_SIZE];
    bzero(output, MAX_SIZE);
    cout << "output orig: " << output << endl;
	for (auto const& user : ACTIVE_SOCKETS) {

        cout << "sock: " << user.first << "name: " << user.second << endl;

		// Skip current user 
		if (user.first == sockfd)
			continue; 
		
		// Append to output 
		char temp[MAX_SIZE]; 
		sprintf(temp, "%s\n", user.second); 
		strcat(output, temp); 

		//TODO appending an extra new line character
	}
     
    cout << "about to send users ** " << output << endl;
	// Send active users to client 
	if (!send_msg('U', sockfd, output, "Error sending active users to client")) 
    	return;  
}


int is_active(char* username) {
    cout << "usrname in is_active: " << username << endl;
	for (auto const& user : ACTIVE_SOCKETS) {
        cout << "key: " << user.first << " value: " << user.second << endl;
		if (strcmp(user.second, username) == 0) {
			return user.first;  
		}
	}	
	return -1;
    }
void broadcast(int sockfd) {
    cout << "Entered broadcast function " << endl;
    
    char acknowledgement[] = "ZB";
    
    //First, send acknowledgement
    if(!send_msg('Z', sockfd, acknowledgement, "Error sending ack  message to  user"))
        return; 
    
    // Receive message to send 
    
    char msg[MAX_SIZE]; 
    //string msg;
    if(recv(sockfd, &msg, sizeof(msg), 0) == -1) {
            cout << "Error receiving broadcast message\n"; 
    }
    
    char conf[]= "LB";
    //Send message to all users

    for (auto const& user : ACTIVE_SOCKETS) {
        // Skip current user 
        if (user.first == sockfd){
            continue; 
        }
        // Append to output 
    	char temp[MAX_SIZE]; 
    
        sprintf(temp, "%s\n", user.second);
        //Send message
        if(!send_msg('B', user.first, msg, "Error sending broadcast message to user"))
            return;
        //Send confirmation that message was sent
        if(!send_msg('L', sockfd, conf, "Error sending conf  message to  user"))
            cout << "error\n" << endl; 
    }

}

/* Private message */ 
void private_message(int sockfd) {

	// Send active users to client 
	send_active_users(sockfd);
    
    cout << "sent users to client!!" << endl; 
	
	// Get target user
	char target[MAX_SIZE]; 
	if(recv(sockfd, &target, sizeof(target), 0) == -1) {
		perror("Error receiving target username\n"); 
	}

	// Send public key to client
    char* key = CLIENT_KEYS[sockfd];
    if(!send_msg('C', sockfd, key, "Error sending user's public key to client.")) 
		return; 
		
	// Receive private message to send 
	char msg[MAX_SIZE]; 
	if(recv(sockfd, &msg, sizeof(msg), 0) == -1) {
		cout << "Error receiving private message\n"; 
	}

	cout << "private message to send: " << msg << endl; 
    	
	// Check that target user is active 
	string ack; 
    int target_sockfd = is_active(target);
	if (target_sockfd != -1) {

		cout << "Target sockfd: " << target_sockfd << endl; 

		// Send message to target user 
		if(!send_msg('D', target_sockfd, msg, "Error sending private message to target user"))
			return; 
		ack = "1"; 
	} else {
		ack = "0"; 
	}

	// Send confirmation  FIXME: add back in 
	/*char ack_fm[MAX_SIZE]; 
	strcpy(ack_fm, ack.c_str()); 
	if(!send_msg('C', sockfd, ack_fm, "Error sending confirmation"))
		return;

    cout << "sent confirmation to the user!!" << endl;*/
}

/* Return true if the input file exists */
bool file_exist(char* filename) {
	struct stat s;
	if (stat(filename, &s) < 0) {
		return false; 					    
	}
	return true; 		
}

/* Authenticate user */ 
char* authenticate(char* username) {

    //Check if users.txt exists, and if not, create it
	FILE *user_file; 
	char filename[] = "users.txt"; 
	if(!file_exist(filename)) {
		user_file = fopen("users.txt", "w+"); 
	} else {
		user_file = fopen("users.txt", "r"); 
	}

	if (!user_file) {
		perror("Could not open users file\n"); 
		exit(-1); 
	} 

	// Check if user exists 
	char line[MAX_SIZE];
	char* file_user; 
	char* password; 
	while (fgets(line, MAX_SIZE, user_file)) {
		file_user = strtok(line, ","); 
		if (strcmp(username, file_user) == 0) {
            password = strtok(NULL, ",");
            fclose(user_file);
			return password; // username exists 
		}
	}
    
    fclose(user_file);
	return NULL; // username does not exist	
}

/* Login user */
void login(char* username, char* filePassword, char* password, int new_sockfd) {
    int incorrect;
    while (strcmp(password, filePassword) != 0) { // Passwords do not match.
        incorrect = htonl(1);
        if (send(new_sockfd, &incorrect, sizeof(incorrect), 0) == -1) {
            perror("Error sending password code to client");	
	    }

        // Receive new password attempt from client
        char* encrypted_password = (char*) calloc(MAX_SIZE, sizeof(char));  // Allocate large buffer
	    if (recv(new_sockfd, encrypted_password, MAX_SIZE, 0) == -1) {
		    perror("Error receiving encrypted password from client\n"); 
	    }
   
        password = decrypt(encrypted_password);
        free(encrypted_password);
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
    // Add socket to map of active client sockets   
    int new_sockfd = *(int*) socket_desc;
    cout << "In handle! " << new_sockfd << endl;
    ACTIVE_SOCKETS[new_sockfd] = NULL;

    char user[MAX_SIZE];
	char* pubKey = getPubKey(); 

	// Receive username from client
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
    if (filePassword) { 
	    user_exists = htonl(1);
    }
    else {
        user_exists = htonl(0);
    }

    if (send(new_sockfd, &user_exists, sizeof(user_exists), 0) == -1) {
        perror("Error sending user authentication to client");
    } 

	// Receive password from client 
	char encrypted_password[MAX_SIZE]; 
	if (recv(new_sockfd, &encrypted_password, sizeof(encrypted_password), 0) == -1) {
		perror("Error receiving encrypted password from client\n"); 
	}
	char* password = decrypt(encrypted_password); 

    // Login or register user 
    if (user_exists) {
        login(user, filePassword, password, new_sockfd);
    } 
    else {
        addUser(user, password);
    }

    // Add user to active socket map
    ACTIVE_SOCKETS[new_sockfd] = user;

	// Send ack back to client
	int ack = htonl(1); 
	if (send(new_sockfd, &ack, sizeof(ack),0) == -1) {
		perror("Error sending ack to client\n"); 
	}

	// Receive client pubkey
	char client_pubkey[MAX_SIZE]; 
	if (recv(new_sockfd, &client_pubkey, sizeof(client_pubkey), 0) == -1) {
		perror("Error receiving client pubkey\n"); 
	}
	CLIENT_KEYS[new_sockfd] = client_pubkey; 

	cout << CLIENT_KEYS[new_sockfd] << endl; 

    // Listen for commands from client
    while (1) {
        cout << "waiting for cmd from client..." << endl;
        char command[MAX_SIZE];
        if (recv(new_sockfd, &command, sizeof(command), 0) == -1) {
            perror("Error receving command from client");
        }
        cout << "got " << command << " from client" << endl;
        if (strcmp(command, "BM") == 0) {
            broadcast(new_sockfd);
        }
        else if (strcmp(command, "PM") == 0) {
			private_message(new_sockfd); 
		}
        else if (strcmp(command, "EX") == 0) {
            // Close socket descriptor
            close(new_sockfd); 
            // Remove user from active user map
            ACTIVE_SOCKETS.erase(new_sockfd);
            break;
        }
        else {
            perror("Invalid command.");
        }
         
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

        new_sockfd = accept(sockfd, (struct sockaddr*) &client_sock, &len);
        cout << "sock: " << new_sockfd << endl;        
        if (new_sockfd < 0) {
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
