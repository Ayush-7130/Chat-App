#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <semaphore.h>
#include "rc.h"

#define PORT 5000
// #define MAX_MESSAGE_SIZE 1024
#define GOODBYE "GOODBYE!"
char *private_key;

typedef struct msg
{
	char *msgLine;
	struct msg *next;
} msg;

typedef struct list
{
	struct msg *front, *rear;
	int count;
} list;

pthread_t sender, receiver;
int isOpen = 0, isChat = 0, partner = -1, server_client_fd;
sem_t mutex, accessList, prt;
// int messageSize = sizeof(char) * MAX_MESSAGE_SIZE;
list *msgs;

int Initiate_Client_Socket()
{
	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	char *serverIPAddress = "127.0.0.1";

	if (client_fd < 0)
	{
		printf("\033[1;31;40m");
		printf("Socket creation failed.\n");
		printf("\033[0;37;40m");
		exit(1);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);
	inet_pton(AF_INET, serverIPAddress, &server_address.sin_addr);

	int connection = connect(client_fd, (struct sockaddr *)&server_address, sizeof(server_address));

	if (connection < 0)
	{
		printf("\033[1;31;40m");
		printf("connect() failed\n");
		printf("\033[0;37;40m");
		exit(1);
	}

	isOpen = 1;
	printf("\033[1;32;40m");
	printf("Connected to server\n");
	printf("\033[0;37;40m");
	return client_fd;
}

list *Create_List()
{
	list *l = (list *)malloc(sizeof(list));
	l->front = l->rear = NULL;
	l->count = 0;
	return l;
}

void print(char *msg)
{
	printf("\033[1;32;40m");
	for (int i = 3; i < strlen(msg); i++)
	{
		printf("%c", msg[i]);
	}
	printf("\n");
	printf("\033[0;37;40m");
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

void start_terminal()
{
	int terminalWidth = getTerminalWidth();

	for (int i = 0; i < terminalWidth; i++)
	{
		printf("-");
	}

	printf("\n");
}

void middle_terminal(char *message)
{
	int terminalWidth = getTerminalWidth();
	int messageLength = strlen(message);
	int remainingWidth = terminalWidth - messageLength;
	int leftPadding = remainingWidth / 2;

	for (int i = 0; i < leftPadding; i++)
	{
		printf("-");
	}

	printf("%s", message);

	for (int i = 0; i < remainingWidth - leftPadding; i++)
	{
		printf("-");
	}
}

void last_terminal(char *message)
{
	int terminalWidth = getTerminalWidth();
	int messageLength = strlen(message);
	int remainingWidth = terminalWidth - messageLength;

	for (int i = 0; i < remainingWidth; i++)
	{
		printf(" ");
	}

	print(message);
}

void Display_Messages(list *l)
{
	msg *temp = l->front;
	char s;
	int x;

	while (temp != NULL)
	{
		s = temp->msgLine[0];
		x = s - 48;
		if (x == partner)
			print(temp->msgLine);
		else
		{
			last_terminal(temp->msgLine);
		}
		temp = temp->next;
	}

	free(temp);
}

void Add_Messages_List(list *msgList, char *original, int sender)
{
	int shouldAdd = 1;
	if (!shouldAdd)
	{
		return;
	}

	msg *incomingMsg = (msg *)malloc(sizeof(msg));
	incomingMsg->next = NULL;
	incomingMsg->msgLine = (char *)malloc(messageSize);
	sprintf(incomingMsg->msgLine, "%d: %s", sender, original);

	if (msgList->rear == NULL)
	{
		msgList->front = msgList->rear = incomingMsg;
		msgList->rear = incomingMsg;
		return;
	}

	msgList->rear->next = incomingMsg;
	msgList->rear = incomingMsg;
	msgList->count += 1;
}

void Refresh_Display()
{
	system("clear");
	middle_terminal("Chat Records");
	Display_Messages(msgs);
	start_terminal();
}

void *send_thread(void *arg)
{
	int *fd = (int *)arg;
	int client_fd = *fd;

	printf("Enter command to interact with the server.\n");
	printf("Supported Commands:\n");
	printf("1. STATUS - To get the list of available clients\n");
	printf("2. CHAT 'Clientid' - Chat with the client of given id\n");
	printf("3. CLOSE - Close the connection\n\n");

	char *message = (char *)malloc(messageSize);
	char *reply = (char *)malloc(messageSize);
	char *secure_reply = (char *)malloc(messageSize);

	while (1)
	{
		memset(reply, 0, sizeof(reply));

		if (isChat)
		{
			memset(secure_reply, 0, sizeof(secure_reply));

			scanf("%[^\n]%*c", reply);
			// printf("\nSent Message: %s\n", reply);

			sem_wait(&accessList);

			secure_reply = encryption(reply, private_key);
			// secure_reply = reply;

			Add_Messages_List(msgs, reply, server_client_fd);
			Refresh_Display();
			sem_post(&accessList);

			if (strcmp(reply, GOODBYE) == 0)
			{
				send(client_fd, reply, messageSize, 0);
				printf("\033[1;32;40m");
				printf("Ending the conversation with %d\n", partner);
				printf("\033[0;37;40m");
				isChat = 0;
				partner = -1;
				free(msgs);

				printf("Enter command to interact with the server.\n");
				printf("Supported Commands:\n");
				printf("1. STATUS - To get the list of available clients\n");
				printf("2. CHAT 'Clientid' - Chat with the client of given id\n");
				printf("3. CLOSE - Close the connection\n\n");

				continue;
			}

			else if (strlen(reply) - 1 > 0)
			{
				send(client_fd, secure_reply, messageSize, 0);
			}
		}
		else
		{
			usleep(700);
			printf("Enter command: ");
			scanf("%[^\n]%*c", reply);
			if (strlen(reply) - 1 > 0)
			{
				printf("%s\n", reply);
				send(client_fd, reply, messageSize, 0);
			}
			printf("\n\n");
			start_terminal();
		}

		fflush(stdin);
	}
}

void *recive_thread(void *arg)
{
	int *fd = (int *)arg;
	int client_fd = *fd;
	char *buffer = (char *)malloc(messageSize);
	char *decrypt_buffer = (char *)malloc(messageSize);

	while (1)
	{
		memset(buffer, 0, sizeof(buffer));

		recv(client_fd, buffer, messageSize, 0);
		printf("\nReceived Message: %s\n", buffer);

		if (isChat)
		{
			memset(decrypt_buffer, 0, sizeof(decrypt_buffer));

			if (strcmp(buffer, GOODBYE) == 0)
			{
				printf("\033[1;32;40m");
				printf("Ending the conversation with %d\n", partner);
				printf("\033[0;37;40m");
				isChat = 0;
				partner = -1;
				free(msgs);
				pthread_cancel(sender);
				// pthread_cancel(receiver);
				pthread_create(&sender, NULL, send_thread, &client_fd);
				continue;
			}

			sem_wait(&accessList);

			decrypt_buffer = decryption(buffer, private_key);
			// decrypt_buffer = buffer;
			// printf("hi recive\n");

			Add_Messages_List(msgs, decrypt_buffer, partner);
			Refresh_Display();
			sem_post(&accessList);
		}
		else
		{
			char *temp;
			char *t2 = (char *)malloc(messageSize);
			strcpy(t2, buffer);
			temp = strtok(t2, " ");

			if (strcmp(temp, "CLOSED") == 0)
			{
				sem_wait(&mutex);
				isOpen = 0;
				sem_post(&mutex);
				free(buffer);
				break;
			}

			else if (strcmp(temp, "C_SUCCESS") == 0)
			{
				char *temp2, values[3][10];
				int i = 0;
				printf("%s\n", buffer);
				temp2 = strtok(buffer, " ");
				while (temp2 != NULL)
				{
					strcpy(values[i], temp2);
					i++;
					if (i == 3)
						break;
					temp2 = strtok(NULL, " ");
				}

				partner = atoi(values[1]);
				private_key = values[2];

				isChat = 1;
				msgs = Create_List();
				system("clear");
				printf("\033[1;32;40m");
				printf("Starting conversation with %d\n", partner);
				printf("\033[0;37;40m");
			}
			free(t2);
		}
	}
}

int main(int argc, char const *argv[])
{
	printf("\033c");
	printf("\033[48;2;0;0;0m");
	printf("\033[0;37;40m");
	printf("\033[2J");

	system("clear");

	printf("\033[1;32;40m");
	printf("Initializing chat app client\n");
	printf("\033[0;37;40m");

	int client_fd = Initiate_Client_Socket();

	sem_init(&mutex, 0, 1);
	sem_init(&accessList, 0, 1);
	sem_init(&prt, 0, 1);

	char *buffer = (char *)malloc(messageSize);
	recv(client_fd, buffer, messageSize, 0);
	server_client_fd = atoi(buffer);
	memset(buffer, 0, sizeof(buffer));
	recv(client_fd, buffer, messageSize, 0);

	// printf("yes\n");

	printf("\n%s\n", buffer);
	free(buffer);

	pthread_create(&sender, NULL, send_thread, &client_fd);
	pthread_create(&receiver, NULL, recive_thread, &client_fd);

	while (1)
	{
		int shouldBreak = 0;
		sem_wait(&mutex);
		shouldBreak = !isOpen;
		sem_post(&mutex);
		if (shouldBreak)
		{
			pthread_cancel(sender);
			pthread_cancel(receiver);
			break;
		}
	}

	printf("\033[1;32;40m");
	printf("Closed connection with server.\n");
	printf("\033[0;37;40m");

	sem_destroy(&mutex);
	sem_destroy(&accessList);
	close(client_fd);
	return 0;
}