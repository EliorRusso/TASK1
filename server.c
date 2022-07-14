
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include "color.h"
#include "server.h"
#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define PortNumber 4015
static int k = 0;

static unsigned int cli_count = 0;
static int uid = 10;
/* Client structure */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
	char color[9];
	char room[32];
} client_t;
static char r1[3];
void red () {
  printf("\033[1;31m");
}
char Colors[5][9] = {"\033[0;35m", "\033[1;32m", "\033[1;33m", "\033[0;31m", "\033[0;35m"};
client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


/* Add clients to queue */
void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);
	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}
char* DivideRoom(char Jh[32]){
	int k = 0;
	int counter = 0;
	for(int i = 0; i < 32; i++){
		if(Jh[i] != '-'){
			k = i;
			break;
		}
	}
	k++;
	for(int j = k; Jh[j] != '\0'; j++){
		r1[counter++] = Jh[j];
	}
	return r1;
}

/* Send message to all clients except sender */
void send_message(char *s, int uid, char room[32]){
	pthread_mutex_lock(&clients_mutex);
	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(strcmp(clients[i]->room,room) == 0){
				printf("%s", clients[i]->room);
				if(clients[i]->uid != uid){
				//strcat(clients[i],Colors[k]);
				if(send(clients[i]->sockfd, s, strlen(s),0) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}
int SameName(char *s1){
	for(int i = 0; i < MAX_CLIENTS; i++){
		if(strcmp(s1, clients[i]->name)){
				k++;
		}
	}if(k >= 2){
		return 1;
	}else{
		return 0;
	}
}
/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_out[BUFFER_SZ];
	char buff_out1[BUFFER_SZ];
	char name[32];
	int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;

	// Name
	if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1){
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} else{
		send(cli->sockfd, "ok", 2 , 0);
		recv(cli->sockfd, buff_out1, BUFFER_SZ, 0);
		strcpy(cli->room, buff_out1);
		strcpy(cli->name, name);
		strcpy(cli->color, Colors[k]);
		strcat(cli->name, cli->color);
		k++;
		sprintf(buff_out, "%s has joined\n", cli->name);
		printf("%s", buff_out);
		send_message(buff_out, cli->uid,cli->room);
	}

	bzero(buff_out, BUFFER_SZ);

	while(1){
		if (leave_flag) {
			break;
		}
		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive > 0){
			if(strlen(buff_out) > 0){
				send_message(buff_out, cli->uid,cli->room);
				printf("%s -> %s\n", buff_out, cli->name);
			}
		} else if (receive == 0 || strcmp(buff_out, "exit") == 0){
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message(buff_out, cli->uid, cli->room);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
	}

  /* Delete client from queue and yield thread */
	close(cli->sockfd);
  queue_remove(cli->uid);
  free(cli);
  cli_count--;
  pthread_join(pthread_self()); //switch with join

	return NULL;
}
void SetSocket(){
  	/* Socket settings */
  	listenfd = socket(AF_INET, SOCK_STREAM, 0);
  	serv_addr.sin_family = AF_INET;
  	serv_addr.sin_addr.s_addr = inet_addr(ip);
  	serv_addr.sin_port = htons(port);
}
void BindSocket(){
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    	perror("ERROR: Socket binding failed");
    	return EXIT_FAILURE;
  	}
}
void ListenToSocket(){
  if (listen(listenfd, 10) < 0) {
    perror("ERROR: Socket listening failed");
    return EXIT_FAILURE;
	}

	printf("=== WELCOME TO THE CHATROOM ===\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);


		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

	}
}
void Destroy(){
	pthread_mutex_destroy(clients_mutex); 
}
int main(){

	char *ip = "127.0.0.1";
	int port = PortNumber;
	int option = 1;
	int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t tid;

  SetSocket();
  BindSocket();
  ListenToSocket();
  Destroy();
	return EXIT_SUCCESS;
}