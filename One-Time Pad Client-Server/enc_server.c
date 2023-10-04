#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#define MAX_CHILDREN 5
#define BUFFER_SIZE 4
#define WRONG_CLIENT 'w'
#define TERMINATION_SIGNAL 't'
#define RESPONSE_CHARACTER 'c'

char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

void error(const char *msg) {
  perror(msg);
  exit(1);
}

/* Handles the communication with a connected client. Reads/processes/sends 
 * data from the client. Checks for termination signals, performs encryption
 * and handles errors. */
void handle_connection(int connectionSocket, const struct sockaddr_in *clientAddress) {
  char buffer[BUFFER_SIZE];

  while (1) {
    memset(buffer, 'a', BUFFER_SIZE - 1);
    int charsRead = recv(connectionSocket, buffer, BUFFER_SIZE - 1, 0);
    
    if (charsRead < 0) {
      error("ERROR reading from socket");
    }

    if (buffer[2] != 'e' && buffer[2] != 'a') {
      // Wrong client connected
      fprintf(stderr, "Wrong client connected. Attempted port: %d\n", ntohs(clientAddress->sin_port));
      buffer[2] = WRONG_CLIENT;
    } else if (strncmp(buffer, "@@e", 3) == 0) {
      // Termination signal received from the client
      buffer[2] = TERMINATION_SIGNAL;
    } else if (buffer[0] != '@' && buffer[1] != '@' && buffer[2] == 'e') {
      // Encryption request received
      int ciphertext_index = (buffer[0] - 'A' + buffer[1] - 'A') % 27;
      buffer[0] = characters[ciphertext_index];
      buffer[1] = RESPONSE_CHARACTER;
      buffer[2] = '\0';
    } else {
      // Invalid or irrelevant data received, continue to next iteration
      continue;
    }

    int charsWritten = send(connectionSocket, buffer, BUFFER_SIZE - 1, 0);
    if (charsWritten < 0) {
      error("ERROR writing to socket");
    }

    if (buffer[2] == WRONG_CLIENT) {
      // Close connection and exit child process
      close(connectionSocket);
      exit(2);
    } else if (buffer[2] == TERMINATION_SIGNAL) {
      // Break out of the loop to stop handling the connection
      break;
    }
  }
  // Close connection and exit child process
  close(connectionSocket);
  exit(0);
}

int main(int argc, char *argv[]) {
  int listenSocket, connectionSocket, child_status;
  int active_connections = 0;
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // Check usage & args
  if (argc < 2) {
    fprintf(stderr, "USAGE: %s port\n", argv[0]);
    exit(1);
  }

  // Create the socket that will listen for connections
  listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  memset((char *)&serverAddress, '\0', sizeof(serverAddress));
  // The address should be network capable
  serverAddress.sin_family = AF_INET;
  // Store the port number
  serverAddress.sin_port = htons(atoi(argv[1]));
  // Allow a client at any address to connect to this server
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5);

  // Accept a connection, blocking if one is not available until one connects
  while (1) {
    while (active_connections == MAX_CHILDREN && waitpid(-1, NULL, 0) > 0) {
      active_connections--;
    }
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
    if (connectionSocket < 0) {
      error("ERROR on accept");
    }

    if (fork() == -1) {
      fprintf(stderr, "fork() failed!");
      break;
    } else if (fork() == 0) {
      // Child process
      handle_connection(connectionSocket, &clientAddress);
    } else {
      // Parent process
      active_connections++;
      waitpid(-1, &child_status, WNOHANG);
    }
  }
  // Close the listening socket
  close(listenSocket);
  return 0;
}

