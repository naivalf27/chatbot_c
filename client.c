#include <stdio.h> //printf
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <unistd.h>
 
int main(int argc , char *argv[]) {
  int sock;
  struct sockaddr_in server;
  char message[1000] , server_reply[2000];
   
  //Create socket
  sock = socket(AF_INET , SOCK_STREAM , 0);
  if (sock == -1) {
    printf("Could not create socket");
  }
  puts("Socket created");
   
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons( 8888 );

  //Connect to remote server
  if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
    perror("connect failed. Error");
    return 1;
  }
   
  puts("Connected\n");
   
  //keep communicating with server
  while(1) {
    memset(message, 0, sizeof(message));
    memset(server_reply, 0, sizeof(server_reply));
    printf("Enter message : ");
    fgets(message,sizeof(message),stdin);
    //Send some data
    if( write(sock , message , sizeof(message)) < 0) {
      puts("Send failed");
      return 1;
    }

    if( read(sock, server_reply, sizeof(server_reply)) < 0) {
      puts("recv failed");
      break;
    }
     
    puts("Server reply :");
    puts(server_reply);
  }
   
  close(sock);
  return 0;
}