#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

int addClient(int client_fd);
void removeClient(int client_idx);
bool isClientListEmpty(void);
void broadcast_msg(char* msg);
void* clientThreadFunc(void* clientSocketPtr);
void get_client_username(int clientSocket, char* username, int username_size);
pthread_attr_t setThreadDetatch(void);
