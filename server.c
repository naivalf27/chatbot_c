//gcc server.c -o server.out -lpthread
#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
 

typedef char bool;
#define true 1
#define false 0

//the thread function
void *connection_handler(void *);

char* bot(const char *);
bool equal(const char *,const char *);


int main(int argc , char *argv[]) {
  int socket_desc , client_sock , c , *new_sock;
  struct sockaddr_in server , client;
   
  //Create socket
  socket_desc = socket(AF_INET , SOCK_STREAM , 0);
  if (socket_desc == -1) {
      printf("Could not create socket");
  }
  puts("Socket created");
   
  //Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( 8888 );
   
  //Bind
  if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
      //print the error message
      perror("bind failed. Error");
      return 1;
  }
  puts("bind done");
   
  //Listen
  listen(socket_desc , 3);
   
  //Accept and incoming connection
  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);
  while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
    puts("Connection accepted");
     
    pthread_t sniffer_thread;
    new_sock = malloc(1);
    *new_sock = client_sock;
     
    if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0) {
        perror("could not create thread");
        return 1;
    }
    puts("Handler assigned");
  }
   
  if (client_sock < 0) {
    perror("accept failed");
    return 1;
  }
   
  return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc) {
  //Get the socket descriptor
  int sock = *(int*)socket_desc;
  int read_size;
  char client_message[2000];


  //Receive a message from client
  while( read(sock, client_message, sizeof(client_message)) > 0) {
    char *t = bot(client_message);
    write(sock , t , strlen(t));
    memset(client_message, 0, sizeof(client_message));
    // memset(t, 0, sizeof(t));
  }
   
  if(read_size == 0) {
      puts("Client disconnected");
      fflush(stdout);
  } else if(read_size == -1) {
      perror("recv failed");
  }
  
  close(sock);
  //Free the socket pointer
  free(socket_desc);
   
  return 0;
}

char* bot(const char* msg) {
  char *message;
  // memset(message, 0, sizeof(message));
  // strcpy(message, "il faut faire ça");
  if(equal(msg,"salut ça va ?")){
    message = "oui et toi ?";
  } else {
    message = "il faut faire ça";
  }
  return message;
}

bool equal(const char *a,const char *b)
{
  unsigned len = strlen(b);
  printf("a = %lu\n", strlen(a));
  printf("b = %lu\n", strlen(b));
  if (strlen(a) != (strlen(b)+1))
    return false;

  for (unsigned i = 0; i < len; i += 1)
  {
    if (a[i] != b[i])
      return false; /* strings are not equal */
  }
  return true;
}
