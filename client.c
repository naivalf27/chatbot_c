#include <stdio.h> //printf
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

int initClient(char type);
char typeClient();
void append(char* s, char c);

int main(int argc , char *argv[]) {

	int type = typeClient();

	return initClient(type);
}

char typeClient() {
	int type = 'p';

	puts("Qui etes-vous ?");
	puts("    1. Un client");
	puts("    2. Un fleuriste");

	do{
		type = getchar();
	}while (type != '1' && type != '2');

	return type;
}

int SOCKET_CLIENT;

int initClient(char type){
	struct sockaddr_in server;
	char message[1000] , server_reply[2000];
	fd_set readfds;
	int max_sd;

	//Create socket
	SOCKET_CLIENT = socket(AF_INET , SOCK_STREAM , 0);
	if (SOCKET_CLIENT == -1) {
		printf("Could not create socket");
	}

	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 8888 );

	//Connect to remote server
	if (connect(SOCKET_CLIENT , (struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("connect failed. Error");
		return 1;
	}

	char initMessage[128] = "{";
	append(initMessage, type);
	append(initMessage, '}');
	if( write(SOCKET_CLIENT , initMessage , sizeof(initMessage)) < 0) {
		puts("Init fail");
		return 1;
	}

	int isFleuriste = type == '2';

	//keep communicating with server
	while(1) {
		FD_ZERO(&readfds);
		//add master socket to set
		FD_SET(SOCKET_CLIENT, &readfds);
		FD_SET(fileno(stdin), &readfds);

		max_sd = (SOCKET_CLIENT > fileno(stdin))?SOCKET_CLIENT:fileno(stdin);

		memset(message, 0, sizeof(message));
		memset(server_reply, 0, sizeof(server_reply));
		//fgets(message,sizeof(message),stdin);

		int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		if ((activity < 0) && (errno!=EINTR))
		{
			printf("select error");
		}

		if (FD_ISSET(fileno(stdin), &readfds)){
			if (read(fileno(stdin), message, sizeof(message)) < 0) {
				puts("fgets failed");
				break;
			}
			if( write(SOCKET_CLIENT , message , sizeof(message)) < 0) {
				puts("Send failed");
				break;
			}
		}

		if (FD_ISSET(SOCKET_CLIENT, &readfds)){
			int result = read(SOCKET_CLIENT, server_reply, sizeof(server_reply));
			if( result < 0) {
				puts("recv failed");
				break;
			} else if (result == 0){
				puts("Server disconnected");
				break;
			} else {
				if (isFleuriste == 1){
					isFleuriste = 0;
					printf("%s",server_reply);
					char c;
					do{
						c = getchar();
					}while ( c < '0' || c > '9' );

					char initMessage[128] = "[";
					append(initMessage, c);
					append(initMessage, ']');
					if( write(SOCKET_CLIENT , initMessage , sizeof(initMessage)) < 0) {
						puts("Init fail");
						return 1;
					}

				} else {
					printf("|----> %s\n", server_reply);
				}
			}
		}
	}

	close(SOCKET_CLIENT);
	return 0;
}

void append(char* s, char c)
{
	int len = strlen(s);
	s[len] = c;
	s[len+1] = '\0';
}
