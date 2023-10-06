#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>

#define PORT 5000
#define MAX_MESSAGE_SIZE 1024
#define MAX_CLIENTS 30
#define CLIENTS_STATUS "STATUS"
#define ESTABLISH_CHAT "CHAT"
#define CONNECTION_CLOSE_COMMAND "CLOSE"
#define GOODBYE_MESSAGE "GOODBYE!"

typedef struct client
{
	int socket_id; // socket ID that client has been assigned
	int status;	   // flag to check if the client is free or not; 1 for free 0 for otherwise
	int partner;   // the device it is connected to
} client;

int totalClients = 0;
client *clientDetails[MAX_CLIENTS];
int messageSize = sizeof(char) * MAX_MESSAGE_SIZE;
sem_t mutex;

client *createClient(int socket_id)
{
	client *newClient = (client *)malloc(sizeof(client));
	newClient->socket_id = socket_id;
	newClient->status = 1;
	newClient->partner = -1;
	return newClient;
}

void destroyClient(client *prevClient)
{
	prevClient->socket_id = -1;
	prevClient->status = -1;
	prevClient->partner = -1;
}

int getTerminalWidth()
{
	FILE *pipe = popen("tput cols", "r");
	if (pipe == NULL)
	{
		printf("Failed to get terminal width.\n");
		exit(1);
	}

	int width;
	fscanf(pipe, "%d", &width);
	pclose(pipe);

	return width;
}

char* middle_terminal(char *message)
{
	int terminalWidth = getTerminalWidth();
	int messageLength = strlen(message);
	int remainingWidth = terminalWidth - messageLength;
	int leftPadding = remainingWidth / 2;

	char *mid_msg = (char *)malloc(terminalWidth * sizeof(char));

	for (int i = 0; i < leftPadding; i++)
	{
		mid_msg[i] = '-';
	}

	strcat(mid_msg, message);

	for (int i = messageLength + leftPadding; i < terminalWidth; i++)
	{
		mid_msg[i] = '-';
	}

	return mid_msg;
}

int generateRandomInteger(int min, int max)
{
	return min + rand() % (max - min + 1);
}

int initSocket()
{
	int server_fd;
	struct sockaddr_in address;

	int opt = 1;
	int addrlen = sizeof(address);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		printf("\033[1;31;40m");
		printf("bind() failed");
    	printf("\033[0;37;40m");
		exit(1);
	}

	if (listen(server_fd, 10) != 0)
	{
		printf("\033[1;31;40m");
		printf("listen() failed\n");
    	printf("\033[0;37;40m");
	}

	return server_fd;
}

char *CONNECTED_CLIENTS()
{
	char *message = (char *)malloc(messageSize);
	char *details = (char *)malloc(messageSize);
	memset(message, 0, sizeof(message));
	memset(details, 0, sizeof(details));

	int i, currentClients = 0;
	sem_wait(&mutex);
	for (i = 0; i < totalClients; i++)
	{
		if (clientDetails[i]->socket_id > 0)
		{
			currentClients++;
			char *temp = (char *)malloc(messageSize);
			sprintf(temp, "Instance ID: %d\tStatus: %s\n", clientDetails[i]->socket_id, clientDetails[i]->status == 1 ? "FREE" : "BUSY");
			strcat(message, temp);
			free(temp);
		}
	}

	sem_post(&mutex);
	sprintf(details, "Total connected clients: %d\n", currentClients);
	strcat(details, message);
	free(message);

	return details;
}

void sendConnectedClientMessage(int socket_id)
{
	char *clientInfoMsg = (char *)malloc(messageSize);
	char *details = CONNECTED_CLIENTS();
	sprintf(clientInfoMsg, "\n\n%s\nYour Instance ID: %d\n\n%s", middle_terminal("Client Details"), socket_id, details);
	send(socket_id, clientInfoMsg, messageSize, 0);
	free(clientInfoMsg);
	free(details);
}

void WELCOME_MESSAGE(int socket_id)
{
	char *welcomeMessage = (char *)malloc(messageSize);
	char *details = CONNECTED_CLIENTS();
	sprintf(welcomeMessage, "Welcome to the Chat App!\nYour Instance ID: %d\n%s\n", socket_id, details);
	send(socket_id, welcomeMessage, messageSize, 0);
	free(welcomeMessage);
	free(details);
}

int MESSAGE_PARSER(int socket_id, char *buffer, int *arg)
{
	char *temp, values[2][10];
	int i = 0;
	temp = strtok(buffer, " ");
	while (temp != NULL)
	{
		strcpy(values[i], temp);
		i++;
		if (i == 2)
			break;
		temp = strtok(NULL, " ");
	}

	if (strcmp(values[0], CLIENTS_STATUS) == 0)
	{
		return 1;
	}
	if (strcmp(values[0], ESTABLISH_CHAT) == 0)
	{
		*arg = atoi(values[1]);
		return 2;
	}
	if (strcmp(values[0], CONNECTION_CLOSE_COMMAND) == 0)
	{
		return 3;
	}
	else
	{
		return 4;
	}
}

int FIND_INDEX(int socket_id)
{
	int index = -1, i;
	sem_wait(&mutex);
	for (i = 0; i < totalClients; i++)
	{
		if (clientDetails[i]->socket_id == socket_id)
		{
			index = i;
			break;
		}
	}

	sem_post(&mutex);
	return index;
}

void ESTABLISHCHAT(int sender_id, int receiver_id, int sender_index)
{
	char *response = (char *)malloc(messageSize);
	int isValid = 1;

	if (receiver_id == sender_id)
	{
		sprintf(response, "You can't chat with yourself!\n");
		isValid = 0;
	}

	int receiver_index = FIND_INDEX(receiver_id);
	if (receiver_index == -1)
	{
		sprintf(response, "Requested Client ID %d is invalid.\n", receiver_id);
		isValid = 0;
	}

	else if (clientDetails[receiver_index]->status != 1)
	{
		sprintf(response, "Requested Client ID %d is not currently available to chat.\n", receiver_id);
		isValid = 0;
	}

	if (!isValid)
	{
		send(sender_id, response, messageSize, 0);
		free(response);
		return;
	}

	int private_key = generateRandomInteger(1, 100);

	clientDetails[receiver_index]->status = 0;
	clientDetails[sender_index]->status = 0;
	clientDetails[receiver_index]->partner = sender_id;
	clientDetails[sender_index]->partner = receiver_id;

	sprintf(response, "C_SUCCESS %d %d", receiver_id, private_key);
	send(sender_id, response, messageSize, 0);

	sprintf(response, "C_SUCCESS %d %d", sender_id, private_key);
	send(receiver_id, response, messageSize, 0);

	free(response);
}

void DESTROY_CHAT(int sender_id, int receiver_id, int sender_index)
{
	char *response = (char *)malloc(messageSize);
	int receiver_index = FIND_INDEX(receiver_id);

	if (receiver_index == -1)
	{
		strcpy(response, "Something went wrong in finidng the index\n");
		send(sender_id, response, messageSize, 0);
		free(response);
		return;
	}

	clientDetails[receiver_index]->status = 1;
	clientDetails[sender_index]->status = 1;
	clientDetails[receiver_index]->partner = -1;
	clientDetails[sender_index]->partner = -1;

	strcpy(response, "C_E_SUCCESS");
	send(sender_id, response, messageSize, 0);
	send(receiver_id, response, messageSize, 0);

	free(response);
}

void *CLIENT_CHAT(void *arg)
{
	int *thread_id = (int *)arg;
	int socket_id = *thread_id;
	int index;

	sem_wait(&mutex);
	index = totalClients++;
	sem_post(&mutex);

	client *newClient = createClient(socket_id);
	clientDetails[index] = newClient;

	char *answer = (char *)malloc(messageSize);
	char *buffer = (char *)malloc(messageSize);
	sprintf(answer, "%d", socket_id);
	send(socket_id, answer, messageSize, 0);
	WELCOME_MESSAGE(socket_id);

	while (1)
	{
		memset(buffer, 0, sizeof(buffer));
		recv(socket_id, buffer, messageSize, 0);

		printf("\033[1;32;40m");
		printf("Client %d:\n", socket_id);
    	printf("\033[0;37;40m");

		int arg = -1;
		if (clientDetails[index]->status == 1)
		{
			int res = MESSAGE_PARSER(socket_id, buffer, &arg);
			if (res == 1)
			{
				sendConnectedClientMessage(socket_id);
			}
			else if (res == 2)
			{
				ESTABLISHCHAT(socket_id, arg, index);
			}
			else if (res == 3)
			{
				break;
			}
			else
			{
				strcpy(answer, "Invalid command!\n");
				send(socket_id, answer, messageSize, 0);
			}
		}
		else
		{
			int partner = clientDetails[index]->partner;
			printf("Message\t%d -> %d: %s\n", socket_id, partner, buffer);

			send(partner, buffer, messageSize, 0);
			if (strcmp(buffer, GOODBYE_MESSAGE) == 0)
			{
				DESTROY_CHAT(socket_id, partner, index);
			}
		}
	}

	printf("\033[1;32;40m");
	printf("Closing connection for - %d\n", socket_id);
    printf("\033[0;37;40m");

	strcpy(answer, "CLOSED");
	send(socket_id, answer, messageSize, 0);
	destroyClient(clientDetails[index]);
	close(socket_id);
	free(answer);
	free(buffer);
	pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
	printf("\033c");
	printf("\033[48;2;0;0;0m");
    printf("\033[0;37;40m");
	printf("\033[2J");

	system("clear");

	printf("\033[1;32;40m");
	printf("Initializing chat app server\n");
    printf("\033[0;37;40m");

	sem_init(&mutex, 0, 1);

	int server_fd = initSocket();
	int socket;

	struct sockaddr_in address;
	int addrlen = sizeof(address);
	pthread_t thread_id;

	while (1)
	{
		socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

		if (socket < 0)
		{
			printf("\033[1;31;40m");
			printf("accept() failed\n");
    		printf("\033[0;37;40m");
			continue;
		}

		printf("\033[1;32;40m");
		printf("Server connected to client - %d\n", socket);
    	printf("\033[0;37;40m");
		pthread_create(&thread_id, NULL, CLIENT_CHAT, &socket);
	}

	sem_destroy(&mutex);
	return 0;
}