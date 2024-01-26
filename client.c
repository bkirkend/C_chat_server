#include "definitions.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

void* receiveMessages(void* arg) {
  int serverSocket = *(int *)arg;
  free(arg);
  char msgbuf[BUFFER_SIZE];
  while (true) {
    memset(msgbuf, 0, BUFFER_SIZE * sizeof(char));
    int num_read;
    //printf("trying to read from server\n");
    if((num_read = read(serverSocket, msgbuf, (BUFFER_SIZE * sizeof(char)) - 1)) <= 0){
      perror("disconnected from socket\n");
      break;
    }
    //printf("after read from server\n");
    msgbuf[num_read] = '\0'; 
    printf("%s\n", msgbuf);
  }
  return NULL;
}

int main() {
  int serverSocket;
  struct sockaddr_in serverAddress;
  struct hostent *host;

  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket == -1) {
    perror("Error in socket creation\n");
    exit(EXIT_FAILURE);
  }

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(PORT);

  // establish connection through localhost
  struct in_addr addr;
  inet_aton("127.0.0.1", &addr);
  memcpy(&serverAddress.sin_addr, &addr, sizeof(addr));

  if (connect(serverSocket, (struct sockaddr *)&serverAddress,
              sizeof(serverAddress)) == -1) {
    perror("Error in connecting to server\n");
    exit(EXIT_FAILURE);
  }
  
  //spawn thread to read all incoming messages
  pthread_t readThread;
  int *socketPtr = malloc(sizeof(int));
  *socketPtr = serverSocket;
  pthread_create(&readThread, NULL, receiveMessages, socketPtr);
  pthread_detach(readThread);
   
  char msgbuf[BUFFER_SIZE];

  while (true) {
    memset(msgbuf, 0, BUFFER_SIZE * sizeof(char));
    fgets(msgbuf, BUFFER_SIZE, stdin);
   
    msgbuf[strcspn(msgbuf, "\n")] = '\0';
    if (strcmp(msgbuf, "exit") == 0) {
      printf("exiting now\n");
      pthread_join(readThread, NULL);
      break;
    }

    //go back to previous line and overwrite
    printf("\033[F");  
    printf("\r");
    for (size_t i = 0; i < strlen(msgbuf); ++i) {
      putchar(' ');  
    }
    printf("\r");  

    if (write(serverSocket, msgbuf, strlen(msgbuf)) == -1) {
      perror("Error in sending message to server\n");
      break;
    }
  }

  close(serverSocket);
  return 0;
}
