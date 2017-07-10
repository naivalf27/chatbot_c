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
#include <sys/time.h>//FD_SET, FD_ISSET, FD_ZERO macros
//#include <json/json.h>


typedef char bool;
#define true 1
#define false 0


//the thread function
void *connection_handler(void *);
void intHandler(int);

int convClient(int numThreadClient, char* message);
int convFleuriste(int numThreadClient, char* message);
int init(int numThreadClient,  char* message);

char* bot(const char *);

bool equal(const char *,const char *);
char* concat(char* str1, char* str2);

struct ThreadClient {
	pthread_t thread;
	int *socket;
	char typeClient;
	int numThread;
	int *socketTied;
};

struct ThreadClient* TAB_THREAD_ARGS;
int MAX_THREADS = 5;
int INDEX_LAST_THREAD = 0;
int SOCKET_SERVER;

int main(int argc , char *argv[]) {
	int client_sock , c , *new_sock;
	struct sockaddr_in server , client;
	
	signal(SIGINT, intHandler);
	
	TAB_THREAD_ARGS = (struct ThreadClient*)malloc(MAX_THREADS*sizeof(struct ThreadClient));
	INDEX_LAST_THREAD = 0;
	
	//Create socket
	SOCKET_SERVER = socket(AF_INET , SOCK_STREAM , 0);
	if (SOCKET_SERVER == -1) {
		puts("Could not create socket");
	}
	puts("Socket created");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8888 );
	
	//Bind
	if( bind(SOCKET_SERVER,(struct sockaddr *)&server , sizeof(server)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
	
	//Listen
	listen(SOCKET_SERVER , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (client_sock = accept(SOCKET_SERVER, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
		puts("Connection accepted");
		new_sock = malloc(1);
		*new_sock = client_sock;
		
		
		
		if (INDEX_LAST_THREAD < MAX_THREADS) {
			struct ThreadClient *args = malloc(sizeof *args);
			if (args != NULL)
			{
				args->socket = new_sock;
				args->numThread = INDEX_LAST_THREAD;
				args->typeClient = 'z';
				args->socketTied = 0;
				
				if( pthread_create( &(args->thread) , NULL ,  connection_handler , args) < 0) {
					perror("could not create thread");
					return 1;
				}
				TAB_THREAD_ARGS[INDEX_LAST_THREAD] = *args;
				INDEX_LAST_THREAD += 1;
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
	for (int i = 0; i<INDEX_LAST_THREAD; i++) {// fermeture de tout les thread & socket ouvert
		close(*TAB_THREAD_ARGS[i].socket);
		pthread_cancel(TAB_THREAD_ARGS[i].thread);
		pthread_join(TAB_THREAD_ARGS[i].thread, NULL);
	}
	close(SOCKET_SERVER);
	free(TAB_THREAD_ARGS);
	fflush(stdout);
	exit(0);
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *context) {
	//Get the socket descriptor
	
	struct ThreadClient *args = context;
	
	int numThread = args->numThread;
	int socket = *(args->socket);
	
	fd_set readfds;
	int max_sd;
	char client_message[2000];
	
	while(true) {
		FD_ZERO(&readfds);
		//add master socket to set
		FD_SET(socket, &readfds);
		max_sd = socket;
		memset(client_message, 0, sizeof(client_message));
		int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		if ((activity < 0) && (errno!=EINTR))
		{
			puts("select error");
		}
		
		if (FD_ISSET(socket, &readfds)){
			int result = read(socket, client_message, sizeof(client_message));
			if( result < 0) {
				puts("recv failed");
				break;
			} else if (result == 0){
				puts("Client disconnected");
				fflush(stdout);
				break;
			} else {
				int res = -1;
				if (TAB_THREAD_ARGS[numThread].typeClient == '1'){// si c'est un client
					res = convClient(numThread, client_message);
				} else if (TAB_THREAD_ARGS[numThread].typeClient == '2' && TAB_THREAD_ARGS[numThread].socketTied != 0){// si c'est un fleuriste et qu'il n'est pas encore relier a un client
					res = convFleuriste(numThread, client_message);
				} else {
					res = init(numThread, client_message);
				}
				if(res == 0){
					break;
				}
			}
		}
		
	}
	
	close(socket);
	//Free the socket pointer
	free(args);
	
	return 0;
}

char* bot(const char* msg) {
	char *message;
	if(equal(msg,"salut ça va ?")){
		message = "oui et toi ?";
	} else {
		message = "il faut faire ça";
	}
	return message;
}

int convClient(int numThreadClient, char* message) {
	struct ThreadClient *args = &TAB_THREAD_ARGS[numThreadClient];
	
	if (args->socketTied == 0) {//Si le client n'est pas relier à un fleuriste
		char *retour_client = bot(message);
		if( write(*(args->socket) , retour_client , strlen(retour_client)) < 0) {
			puts("Send failed");
			return 0;
		}
	} else {// si le client est relier a un fleuriste
		if( write(*(args->socketTied) , message , strlen(message)) < 0) {
			puts("Send failed");
			return 0;
		}
	}
	return 1;
}

int convFleuriste(int numThreadClient, char* message) {
	struct ThreadClient *args = &TAB_THREAD_ARGS[numThreadClient];
	
	if( write(*(args->socketTied) , message , strlen(message)) < 0) {
		puts("Send failed");
		return 0;
	}
	return 1;
}

int init(int numThreadClient,  char* message) {
	struct ThreadClient *args = &TAB_THREAD_ARGS[numThreadClient];
	
	if (message[0] == '{' && message[strlen(message)-1] == '}'){
		args->typeClient = message[1];
		if (message[1] == '2'){
			char * retour = "";
			int count = 1;
			for (int i = 0; i < INDEX_LAST_THREAD; i++) {
				if (TAB_THREAD_ARGS[i].typeClient == '1'){// si c'est un client
					char text[] = "    X. client X\n";
					text[4] = count+'0';
					count++;
					text[14] = i+'0';
					retour = concat(retour, text);
				}
			}
			if( write(*(args->socket) , retour , strlen(retour)) < 0) {
				puts("Send failed");
				return 0;
			}
		} else {
			char * retour = "Client connected";
			
			if( write(*(args->socket) , retour , strlen(retour)) < 0) {
				puts("Send failed");
				return 0;
			}
		}
	} else if (message[0] == '[' && message[strlen(message)-1] == ']'){
		int count = 0;
		for (int i = 0; i < INDEX_LAST_THREAD; i++) {
				if (TAB_THREAD_ARGS[i].typeClient == '1'){//si c'est un client
					char c = count+'1';
					if (c == message[1]){
						TAB_THREAD_ARGS[count].socketTied = args->socket;
						args->socketTied = TAB_THREAD_ARGS[count].socket;
					}
					count = count + 1;
				}
			}
		char * retour = "Fleuriste connected";
		
		if( write(*(args->socket) , retour , strlen(retour)) < 0) {
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
			return false;
	}
	return true;
}

char* concat(char* str1, char* str2){
	char * str3 = (char *) malloc(1 + strlen(str1)+ strlen(str2) );
	strcpy(str3, str1);
	strcat(str3, str2);
	return str3;
}
