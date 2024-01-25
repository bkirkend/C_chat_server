#include "definitions.h"
#include "server_thread.h"

pthread_mutex_t clientListMutex;
int clientList[MAX_CLIENTS];
bool initializedUsername[MAX_CLIENTS];

int addClient(int client_fd){
  for(int i = 0; i < MAX_CLIENTS; i++){
    if(clientList[i] == -1){
      printf("added client at index: %d\n", i);
      clientList[i] = client_fd;  
      return i;
    }
  }
  return -1;
}

void removeClient(int client_idx){
    initializedUsername[client_idx] = false;
    close(clientList[client_idx]);
    clientList[client_idx] = -1;
}

bool isClientListEmpty(void){
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if(clientList[i] != -1){
      return false;
    }
  }  
  return true;
}

void* clientThreadFunc(void* client_idx_ptr){
  int client_idx = *(int *)client_idx_ptr;
  free(client_idx_ptr);
  
  //printf("in client thread function\n");

  char msginpt[BUFFER_SIZE]; 
  char msgotpt[BUFFER_SIZE];
  memset(msginpt, 0, BUFFER_SIZE * sizeof(char));
  memset(msgotpt, 0, BUFFER_SIZE * sizeof(char));
  char username[USERNAME_SIZE];  
  
  printf("trying to get username\n");
  get_client_username(clientList[client_idx], username, USERNAME_SIZE);
  printf("successfully got username\n");
  int username_act_size = strlen(username);
  initializedUsername[client_idx] = true;
  
  //chat loop
  int valread;
  while(true){
    if((valread = read(clientList[client_idx], msginpt, (BUFFER_SIZE - username_act_size) * sizeof(char) - 1)) <= 0){   
      //lock mutex when client leaving
      pthread_mutex_lock(&clientListMutex);
      removeClient(client_idx);
      pthread_mutex_unlock(&clientListMutex);
      break; 
    }
    //general logic - work from here
    msginpt[valread] = '\0';
    sprintf(msgotpt, "[%s]: %s", username, msginpt);
    int msglen = strlen(msgotpt) * sizeof(char);

    //broadcast message to all connected threads
    for(int i = 0; i < MAX_CLIENTS; i++){
      if(clientList[i] != -1){
        if(write(clientList[i], msgotpt, msglen) <= 0){
          perror("broadcasting message to clients");
        }
      }
    }

    //call memset to reset buffers  
    memset(msginpt, 0, BUFFER_SIZE * sizeof(char));
    memset(msgotpt, 0, BUFFER_SIZE * sizeof(char));
    
  }

  return NULL;
}

void get_client_username(int clientSocket, char* username, int username_size){
  char namePrompt[BUFFER_SIZE] = "Enter your username: ";
  int bytes_read = username_size;
  while(bytes_read > username_size - 1){
    if(write(clientSocket, namePrompt, strlen(namePrompt)) < 0){
      perror("writing to client");
    }
    //printf("wrote to client, waiting for read\n");
    
    if((bytes_read = read(clientSocket, username, (username_size * sizeof(char) - 1))) <= 0){
      perror("reading username");
    }
    //printf("after reading username\n");
  }
    //append null terminator to string
    username[bytes_read] = '\0';
}

pthread_attr_t setThreadDetatch(void){
  pthread_attr_t threadAttr;
  pthread_attr_init(&threadAttr);
  pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED); 
  return threadAttr;
}

int main(int argc, char const **argv) {
  //initializations
  int server_fd, new_socket, valread;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[1024] = {0};
  pthread_mutex_init(&clientListMutex, NULL);
  pthread_t threadPool[MAX_CLIENTS];
  int threadIdx = 0;
  pthread_attr_t detatch_attr = setThreadDetatch(); 
  memset(clientList, -1, MAX_CLIENTS * sizeof(int));
  memset(initializedUsername, false, MAX_CLIENTS * sizeof(bool));

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Forcefully attaching socket to the port 8080
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY; address.sin_port = htons(PORT);

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  
  bool first_connect = true;
  while(true){
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    //printf("socket connected\n");
    pthread_mutex_lock(&clientListMutex);
    int client_idx = addClient(new_socket);
    //printf("client idx: %d\n", client_idx);
    //printf("client id: %d\n", clientList[client_idx]);
    pthread_mutex_unlock(&clientListMutex);
    if(client_idx != -1){
      first_connect = false;
      int* client_idx_ptr = malloc(sizeof(int));
      *client_idx_ptr = client_idx;
      pthread_create(&threadPool[threadIdx], &detatch_attr, clientThreadFunc, (void *)client_idx_ptr);
    }   
    //printf("is client list empty: %d\n", isClientListEmpty());
    if(!first_connect && isClientListEmpty()){
      break;
    }
  }
  
  pthread_mutex_destroy(&clientListMutex);
  // closing the listening socket
  shutdown(server_fd, SHUT_RDWR);
  return 0;
}
