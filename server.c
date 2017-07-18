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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <time.h>
//#include <json/json.h>


typedef char bool;
typedef unsigned char xmlChar;
typedef char * (*fct_compare)(xmlNodePtr, char * message);
typedef void (*fct_browse)(xmlNodePtr, char * message);

#define true 1
#define false 0
#define BAD_CAST (xmlChar *)

xmlNodePtr create_node_client(char * type, char *clientDiscussion);
xmlNodePtr create_node_bot(char * type, char *chatBotResponse);

//the thread function
void *connection_handler(void *);
void intHandler(int);
void print_node(xmlNodePtr noeud, char * message);
void delay(int milliseconds);
void clear_response();

int convClient(int numThreadClient, char* message);
int convFleuriste(int numThreadClient, char* message);
int init(int numThreadClient,  char* message);
int LaunchDataSet();

char * bot(const char *);
char * concat(char* str1, char* str2);
char * compare_node(xmlNodePtr noeud, char * message);
char * convert_xmlChar(xmlChar * xml);
char * convert_constchar(const char * msg);

bool equal(const char *,const char *);

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

xmlDocPtr doc;
xmlNodePtr root;

char* NoResult = "NoResult";
//char* Command = "Open";
char* Response;
char * Type;
char * clientResponse;
char * botResponse;

int Flag = 0;

int main(int argc , char *argv[]) {

	//Lancement de la machine learning
	int result = LaunchDataSet();
	if(result == EXIT_SUCCESS){
		puts("Opening xml file : Complete.");
	}else{
		puts("Opening xml file : Error.");
	}

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

char * convert_xmlChar(xmlChar * xml){
	char * returnMessage =malloc(xmlStrlen(xml) * 1024);
	for(int i = 0; i < xmlStrlen(xml); i++) {
		returnMessage[i] = xml[i];
	}
	return returnMessage;
}

void clear_response(){
	Response = malloc(sizeof(int));
	if(Response != NULL && sizeof(Response) > 0){
		for(int i = 0; i < strlen(Response); i++) {
			Response[i] = '\0';
		}
	}
}

void prefix_search(xmlNodePtr noeud, char * message, fct_browse f) {
    xmlNodePtr n;
    for (n = noeud; n != NULL; n = n->next) {
        f(n, message);
        if ((n->type == XML_ELEMENT_NODE) && (n->children != NULL)) {
            prefix_search(n->children, message, f);
        }
    }
}

char * prefix_search_compare(xmlNodePtr noeud, char * message, fct_compare f) {
    xmlNodePtr n;
	  char * m;
    for (n = noeud; n != NULL; n = n->next) {
        m = f(n, message);

				if(m != NoResult){
					Response = m;
					break;
				}
	        else if ((n->type == XML_ELEMENT_NODE) && (n->children != NULL)) {
	             prefix_search_compare(n->children, message, f);
	        }
						else{
							Response = m;
							return Response;
						}
    }
		return Response;
}

char * compare_node(xmlNodePtr noeud, char * message) {
    if (noeud->type == XML_ELEMENT_NODE) {
        if (noeud->children != NULL && noeud->children->type == XML_TEXT_NODE) {

            xmlChar *contenu = xmlNodeGetContent(noeud);

						char * t = strdup(message);

 						//Conversion du message en xmlChar
 						xmlChar * mess = 	xmlCharStrndup(t, strlen(message)-1);

						//test de la chaine saisie par l'utilisateur avec les données du fichier xml
						int	test = xmlStrEqual(contenu, mess);
						if(test == 1){
							xmlNodePtr next = noeud->next->children;
							contenu = xmlNodeGetContent(next);

							printf("\n Message trouvé %s", contenu);

							char* testChar = convert_xmlChar(contenu);
							Response = testChar;
							return Response;
						}

						return NoResult;
        }
    }
		return NoResult;
}

void print_node(xmlNodePtr noeud, char * message) {

    if (noeud->type == XML_ELEMENT_NODE) {
        xmlChar *chemin = xmlGetNodePath(noeud);
        if (noeud->children != NULL && noeud->children->type == XML_TEXT_NODE) {

            xmlChar * contenu = xmlNodeGetContent(noeud);
            printf("%s -> %s\n", chemin, contenu);
					 char * t = strdup(message);
						//Conversion du message en xmlChar
						xmlChar * mess = 	xmlCharStrndup(t, strlen(message)-1);

						//test de la chaine saisie par l'utilisateur avec les données du fichier xml
						int	test = xmlStrEqual(mess, contenu);
						printf("%s\n", contenu);
						printf("%s\n", mess);

						printf("%d\n", test);
						if(test == 1){
						}
						else{
							Response = NoResult;
					}
        }
    }
}

xmlNodePtr create_node_client(char * type, char *clientDiscussion ){
    xmlNodePtr node_client;

		printf("\n Création du noeud client");
    // Création du noeud selon le type choisi par le client
    node_client = xmlNewNode(NULL, BAD_CAST type);

		printf("\n Création du content client");
		xmlNodeSetContent(node_client, BAD_CAST clientDiscussion);

    return node_client;
}

xmlNodePtr create_node_bot(char * type, char *chatBotResponse){
    xmlNodePtr node_bot;
		printf("\n Création du noeud bot");

		node_bot = xmlNewNode(NULL, BAD_CAST type);

		printf("\n Création du noeud bot");
		xmlNodeSetContent(node_bot, BAD_CAST chatBotResponse);
    return node_bot;
}

// Ouverture du document xml
int LaunchDataSet()
{
    xmlKeepBlanksDefault(0); // Ignore les noeuds texte composant la mise en forme
    doc = xmlParseFile("xml/dataset.xml");
    if (doc == NULL) {
        fprintf(stderr, "Invalid Xml Document\n");
        return EXIT_FAILURE;
    }
    // Récupération de la racine
    root = xmlDocGetRootElement(doc);
    if (root == NULL) {
        fprintf(stderr, "Blank Xml Document\n");
        xmlFreeDoc(doc);
        return EXIT_FAILURE;
    }

	 return EXIT_SUCCESS;
}

void intHandler(int dummy) {
	printf("%s\n", "Ctrl+c");
	for (int i = 0; i<INDEX_LAST_THREAD; i++) {// fermeture de tout les thread & socket ouvert
		close(*TAB_THREAD_ARGS[i].socket);
		pthread_cancel(TAB_THREAD_ARGS[i].thread);
		pthread_join(TAB_THREAD_ARGS[i].thread, NULL);
	}
	xmlFreeDoc(doc);
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

char * convert_constchar(const char * msg){
	char * newChar = malloc(sizeof(msg) * 1024);
	//printf("\n Fonction Convert_constchar, Valeur msg et taille : %s %d", msg, length);
	for(int i = 0; i < strlen(msg)-1; i++){
		newChar[i] = msg[i];
	}
	return newChar;
}

char* bot(const char* msg) {
	char *message;

  clear_response();

	if(Flag == 0){
		char * tt = strdup(msg);
  	message = prefix_search_compare(root, tt, compare_node);
			if(message == NoResult){
		 	 message = "Je n'ai pas compris, voulez-vous améliorer mes réponses en précisant quel est le context de votre précédente réponse ? oui / non";
			 Flag = 1;
		  }
			printf("\n\n");
			return message;
		}

	else if(Flag == 1){
		char * tt = convert_constchar(msg);
		//printf("\n flag 1");
		puts(tt);
		int val = strcmp(tt, "oui");
		if (val == 0) {
			message = "Quel est le type de context ? greetings, expression, booking, sales ...";
			Flag = 2;
			return message;
			}
			else{
				Flag = 0;
				message = "Que puis-je faire pour vous ?";
				return message;
			}
	}
		else if(Flag == 2){
		 Type = strdup(msg);
		 message = "Quel était votre question ou réponse ?";
		 Flag = 3;
		 return message;
		}
			else if(Flag == 3){
				clientResponse = strdup(msg);
				message = "Enfin, quelle est la réponse appropriée à votre demande ?";
				Flag = 4;
				return message;
			}
			else{
					botResponse = strdup(msg);
					message = "Merci pour votre aide, je me sens plus intelligent désormais !";
					xmlKeepBlanksDefault(0);
					Flag = 0;
					if (clientResponse[strlen(clientResponse)-1] == '\n') {
						clientResponse[strlen(clientResponse)-1] = '\0';
					}
					if (Type[strlen(Type)-1] == '\n') {
						Type[strlen(Type)-1] = '\0';
					}
					xmlNodePtr new_node_client = create_node_client(Type, clientResponse);
					if (new_node_client) {
							xmlAddChildList(root, new_node_client);
							printf("\nAdd Node Client : Complete.");
					}

					if (botResponse[strlen(botResponse)-1] == '\n') {
						botResponse[strlen(botResponse)-1] = '\0';
					}
					xmlNodePtr new_node_bot = create_node_bot(Type, botResponse);
					if (new_node_client) {
							xmlAddChildList(root, new_node_bot);
							printf("\nAdd Node Bot: Complete.");
							int retour = xmlSaveFormatFile("xml/dataset.xml", doc, 1);
							if(retour != -1){
								printf("\nSave Xml file : Complete.");
								//xmlDocFormatDump(stdout, doc, 1);
							}
					}
					int result = LaunchDataSet();
					if(result == EXIT_SUCCESS){
						puts("Opening xml file : Complete.");
					}else{
						puts("Opening xml file : Error.");
					}
					return message;
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

void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}
