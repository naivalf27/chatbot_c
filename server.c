//gcc server.c -o server.out -lpthread
#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <signal.h>
#include <errno.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros


typedef char bool;
#define true 1
#define false 0

//the thread function
void *connection_handler(void *);
void intHandler(int);

int convClient(int sock, char* message);
int convFleuriste(int sock, int client);
int init(int sock, int index,  char* message);

char* bot(const char *);
bool equal(const char *,const char *);
void append(char* s, char c);
char* concat(char* str1, char* str2);

struct thread_args {
	pthread_t thread;
	int *sock;
	char type;
	int index;
	int *relier;
};

struct thread_args* threads;
int position, socket_desc;

int main(int argc , char *argv[]) {
	int client_sock , c , *new_sock;
	struct sockaddr_in server , client;
	
	signal(SIGINT, intHandler);
	
	int nbThread = 5; // A remplir avec ton nombre de threads
	threads = (struct thread_args*)malloc(nbThread*sizeof(struct thread_args));
	position = 0;
	
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
		new_sock = malloc(1);
		*new_sock = client_sock;
		
		
		
		if (position < nbThread) {
			struct thread_args *args = malloc(sizeof *args);
			if (args != NULL)
			{
				args->sock = new_sock;
				args->index = position;
				args->type = 'z';
				args->relier = 0;
				
				if( pthread_create( &(args->thread) , NULL ,  connection_handler , args) < 0) {
					perror("could not create thread");
					return 1;
				}
				threads[position] = *args;
				position += 1;
			}
		}
		puts("Handler assigned");
	}
	
	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}
	
	return 0;
}

void intHandler(int dummy) {
	printf("%s\n", "Ctrl+c");
	for (int i = 0; i<position; i++) {
		struct thread_args args = threads[i];
		pthread_t thread = args.thread;
		printf( "%p\n", args.thread );
		int sock = *(int*)args.sock;
		printf("sock = %d\n", sock);
		close(sock);
		pthread_cancel(thread);
		pthread_join(thread, NULL);
	}
	close(socket_desc);
	free(threads);
	fflush(stdout);
	exit(0);
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *context) {
	//Get the socket descriptor
	
	struct thread_args *args = context;
	
	int index = args->index;
	int sock = *(int*)args->sock;
	
	fd_set readfds;
	int max_sd;
	char client_message[2000];
	
	while(true) {
		FD_ZERO(&readfds);
		//add master socket to set
		FD_SET(sock, &readfds);
		max_sd = sock;
		memset(client_message, 0, sizeof(client_message));
		int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		if ((activity < 0) && (errno!=EINTR))
		{
			printf("select error");
		}
		
		if (FD_ISSET(sock, &readfds)){
			int result = read(sock, client_message, sizeof(client_message));
			if( result < 0) {
				puts("recv failed");
				break;
			} else if (result == 0){
				puts("Client disconnected");
				fflush(stdout);
				break;
			} else {
				int res = -1;
				printf("relier = %p\n", threads[index].relier);
				if (threads[index].type == '1'){
					res = convClient(sock, client_message);
				} else if (threads[index].type == '2' && threads[index].relier != 0){
					res = convFleuriste(sock, *threads[index].relier);
				} else {
					res = init(sock, index, client_message);
				}
				if(res == 0){
					break;
				}
			}
		}
		
	}
	
	close(sock);
	//Free the socket pointer
	free(args);
	
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

int convClient(int sock, char* message) {
	char *retour_client = bot(message);
	if( write(sock , retour_client , strlen(retour_client)) < 0) {
		puts("Send failed");
		return 0;
	}
	return 1;
}

int convFleuriste(int sock, int client) {
	char * retour = "Message fleuriste";
	printf("salut je suis un fleuriste");
	if( write(client , retour , strlen(retour)) < 0) {
		puts("Send failed");
		return 0;
	}
	return 1;
}

int init(int sock, int index,  char* message) {
	if (message[0] == '{' && message[strlen(message)-1] == '}'){
		threads[index].type = message[1];
		if (message[1] == '2'){
			char * retour = "";
			int count = 1;
			for (int i = 0; i < position; i++) {
				if (threads[i].type == '1'){
					char text[] = "    X. client X\n";
					text[4] = count+'0';
					count++;
					text[14] = i+'0';
					retour = concat(retour, text);
				}
			}
			if( write(sock , retour , strlen(retour)) < 0) {
				puts("Send failed");
				return 0;
			}
		} else {
			char * retour = "Client connected";
			
			if( write(sock , retour , strlen(retour)) < 0) {
				puts("Send failed");
				return 0;
			}
		}
	} else if (message[0] == '[' && message[strlen(message)-1] == ']'){
		int count = 0;
		for (int i = 0; i < position; i++) {
				if (threads[i].type == '1'){
					printf("%c\n", count+'0');
					char c = count+'1';
					if (c == message[1]){
						threads[index].relier = threads[count].sock;
					}
					count = count + 1;
				}
			}
		//threads[index].relier = message[1];
		
		char * retour = "Fleuriste connected";
		
		if( write(sock , retour , strlen(retour)) < 0) {
			puts("Send failed");
			return 0;
		}
	}
	return 1;
}

bool equal(const char *a,const char *b)
{
	unsigned len = strlen(b);
	if (strlen(a) != (strlen(b)+1))
		return false;
	
	for (unsigned i = 0; i < len; i += 1)
	{
		if (a[i] != b[i])
			return false; /* strings are not equal */
	}
	return true;
}

void append(char* s, char c)
{
	int len = strlen(s);
	s[len] = c;
	s[len+1] = '\0';
}

char* concat(char* str1, char* str2){
	char * str3 = (char *) malloc(1 + strlen(str1)+ strlen(str2) );
	strcpy(str3, str1);
	strcat(str3, str2);
	return str3;
}
