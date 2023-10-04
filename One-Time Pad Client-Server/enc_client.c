#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()

#define RESPONSE_WRONG_SERVER 'w'
#define RESPONSE_TERMINATION 't'
#define RESPONSE_CHARACTER 'c'

// Function prototype
int is_valid_character(int character);

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

/* Helper function to send data through the socket. */
int send_helper(int socket, const char* buffer, int buffer_length) {
  int characters_sent = 0;
  while (characters_sent < buffer_length) {
    int remaining_chars = buffer_length - characters_sent;
    int chars_sent = send(socket, buffer + characters_sent, remaining_chars, 0);
    if (chars_sent == -1) {
      return -1;
    }
    characters_sent += chars_sent;
  }
  return characters_sent;
}

int main(int argc, char *argv[]) {
  int socketFD, charsRead, buffer_length;
  struct sockaddr_in serverAddress;
  char buffer[4];
  FILE *plaintext, *key;

  // Check usage & args
  if (argc < 4) {
    fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
    exit(1);
  }

  // Open the plaintext and key files
  plaintext = fopen(argv[1], "r");
  key = fopen(argv[2], "r");
  if (plaintext == NULL || key == NULL) {
    fprintf(stderr, "Error opening files: %s, %s\n", argv[1], argv[2]);
    exit(1);
  }

  // Get the sizes of the plaintext and key files
  fseek(plaintext, 0, SEEK_END);
  fseek(key, 0, SEEK_END);
  long plaintextSize = ftell(plaintext);
  long keySize = ftell(key);
  fseek(plaintext, 0, SEEK_SET);
  fseek(key, 0, SEEK_SET);

  if (plaintextSize > keySize) {
    fprintf(stderr, "The plaintext file is longer than the key file, exiting\n");
    exit(1);
  }

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0) {
    error("CLIENT: ERROR opening socket");
  }

  // Set up the server address structure
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to the server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
    error("CLIENT: ERROR connecting");
  }

  int exitFlag = 0;
  while (!exitFlag) {
    int plaintext_character = fgetc(plaintext);
    int key_character = fgetc(key);

    if (plaintext_character == EOF || key_character == EOF) {
      break;
    }

    // Check if the characters are valid
    if (!is_valid_character(plaintext_character) || !is_valid_character(key_character)) {
      fprintf(stderr,"CLIENT: Issue with character in %s.\n", (is_valid_character(plaintext_character) ? argv[2] : argv[1]));
      exit(1);
    }

    // Prepare the buffer to send
    buffer[2] = 'e';
    buffer[0] = (plaintext_character == '\n' || key_character == '\n') ? '@' : plaintext_character;
    buffer[1] = (plaintext_character == '\n' || key_character == '\n') ? '@' : key_character;
    buffer_length = 3;

    // Send the buffer to the server
    if (send_helper(socketFD, buffer, buffer_length) < 0) {
      error("CLIENT: ERROR writing to socket");
    }

    // Clear the buffer
    memset(buffer, '\0', sizeof(buffer));

    // Receive the response from the server
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
    if (charsRead < 0) {
      error("CLIENT: ERROR reading from socket");
    }

    // Process the response from the server
    switch (buffer[2]) {
      case RESPONSE_WRONG_SERVER:
        fprintf(stderr, "CLIENT: Wrong port: %d\n", atoi(argv[3]));
        exitFlag = 1;
        break;
      case RESPONSE_TERMINATION:
        fprintf(stdout, "\n");
        exitFlag = 1;
        break;
      case RESPONSE_CHARACTER:
        putc(buffer[0], stdout);
        break;
      default:
        break;
    }
  }

  // Close the socket and files
  close(socketFD);
  fclose(key);
  fclose(plaintext);
  return 0;
}

// Function to check if a character is valid (space, newline, or uppercase letter)
int is_valid_character(int character) {
  if (character == 32 || character == 10) {
    return 1;
  }
  if (character >= 65 && character <= 90) {
    return 1;
  }
  return 0;
}

