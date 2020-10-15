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

typedef struct {
	map<int, char*> active_sockets_map;
} active_sockets_struct; 

/* Helper Functions */

/* Sends either a data or command message to the client
 * type - should be D or C
 * return true if the msg was sent successfully 
 */ 
bool send_msg(char type, int sockfd, char* msg, string error) {

	cout << "In send_message \n" << endl; 

    if (type != 'D' && type != 'C') 
        return false;

    // Add type to front of message
    char new_msg[MAX_SIZE];
    sprintf(new_msg, "%c%s", type, msg); 

	//cout << new_msg << endl; 

    // Send message to client
    if (send(sockfd, new_msg, strlen(new_msg) + 1, 0) == -1) {
        cout << error << endl;
        return false;
    } 
    return true;
}


/* Broadcast message to all clients */
void broadcast(int sockfd) {
    cout << "Entered broadcast function " << endl;
    
    int ack = htonl(1);
    char msg[MAX_SIZE];
    // Send acknowledgment
    if (send(sockfd, &ack, sizeof(ack), 0) == -1) {
        perror("Acknowledgement send error\n");
    } 
     
    cout << "Sent ack" << endl;
         
    // Receive message
    if (recv(sockfd, &msg, sizeof(msg), 0) == -1) {
        perror("Receive message error\n");
    } 
    cout << "Message received: hello?" << msg;


    
    // Send message to all clients   // TODO: comment this part back in
    /*for (auto const &user : ACTIVE_SOCKETS) {
        if (user.first == sockfd) { //skip current user
            continue; 
        }
        if(send(user.first, msg, sizeof(msg), 0) == -1) {
            perror("Error sending message\n");
            continue; 	
        }
    }	*/
  


}

void send_active_users(int sockfd) {

	cout << "in send_active_users" << endl; 

	// Send client size of active sockets list 
	//int n_users = ACTIVE_SOCKETS.size() - 1; //remove curr user
	/*n_users = htonl(n_users); 
	if(send(sockfd, &n_users, sizeof(n_users),0) == -1) {
		perror("Error sending list size\n"); 
	}

	// Receive ack from client 
	int ack; 
	if (recv(sockfd, &ack, sizeof(ack), 0) == -1) {
		perror("Error receiving acknowledgement"); 
	}
	ack = ntohl(ack); 
	if (ack == -1) {
		cout << "An unknown error occurred\n"; 
	}*/

	// Send active usernames

	//TODO tell client if there are no active users 
	
	/*for (auto const& user : ACTIVE_SOCKETS) {
		if (user.first == sockfd) { // Skip current user
			continue; 
		}
        if (!send_msg('D', sockfd, user.second, "Error sending username to client")) 
            continue;
        
		if(send(sockfd, user.second, sizeof(user.second), 0) == -1) {
			perror("Error sending username\n");
		   	continue; 	
		}
	} 	*/ 

	// Send active users in a formatted string 
	char output[MAX_SIZE]; 
	for (auto const& user : ACTIVE_SOCKETS) {
		// Skip current user 
		if (user.first == sockfd)
			continue; 
		
		// Append to output 
		char temp[MAX_SIZE]; 
		sprintf(temp, "%s\n", user.second); 
		strcat(output, temp); 

		//TODO appending an extra new line character
	}

	// Send active users to client 
	if (!send_msg('D', sockfd, output, "Error sending active users to client")) 
    	return;  
}

int is_active(char* username) {
	for (auto const& user : ACTIVE_SOCKETS) {
		if (strcmp(user.second, username) == 0) {
			return 1;  
		}
	}	
	return 0;
}

/* Private message */ 
void private_message(int sockfd) {

	// Send active users to client 
	send_active_users(sockfd); 
	
	// Get target user
	char target[MAX_SIZE]; 
	if(recv(sockfd, &target, sizeof(target), 0) == -1) {
		perror("Error receiving target username\n"); 
	}

	cout << "Target user: " << target << endl; 
	
	// Send public key to client
    char* key = getPubKey();
    send_msg('C', sockfd, key, "Error sending public key to client.");
	
	// Receive message to send 
    	
	// Check if username exists
	if (is_active(target)) {
		//user exists 
	} else {
		//user does not exists	
	}
	
	// Send message to target user 
	
	// Send confirmation
}

/* Authenticate user */ 
char* authenticate(char* username) {

    //TODO: check if users.txt exists, and if not, create it

	// Prepare to authenticate 
	FILE *user_file = fopen("users.txt", "r");  // FIXME: This will give an error if the file does not already exist
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
    ACTIVE_SOCKETS[new_sockfd] = NULL;

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

    // Listen for commands from client
    while (1) {
        char command[MAX_SIZE];
        if (recv(new_sockfd, &command, sizeof(command), 0) == -1) {
            perror("Error receving command from client");
        }
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
