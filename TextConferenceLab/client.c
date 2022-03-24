#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdbool.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <poll.h>
#include "message.h"
#include <netdb.h>

#define COMMAND_LEN 100
#define BUFFER_SIZE 1000

// status
bool logged_in = false;
bool in_session = false;

// socket
int sockfd = -1;
int num_bytes;
socklen_t servaddr_len;
struct sockaddr_in servaddr;

// buffer
char buffer[BUFFER_SIZE];

// client source
char client_id[COMMAND_LEN];

void clear_buffer()
{
	// clear the buffer for next message
	for (int i = 0; i < strlen(buffer); i++)
	{
		buffer[i] = '\0';
	}
}

bool send_buffer()
{
	num_bytes = send(sockfd, buffer, sizeof(buffer), 0);

	if (num_bytes >= 0)
	{
		clear_buffer();
		return true;
	}
	else
	{
		fprintf(stderr, "Could not send.\n");
		clear_buffer();
		return false;
	}
	
}

void login(char *client_id, char *password, char *server_ip, char *server_port)
{
	// check for possible errors

	if (client_id == NULL || password == NULL || server_ip == NULL || server_port == NULL)
	{
		printf("incorrect usage of login");
		return;
	}

	struct addrinfo hints;
	struct addrinfo *server_info, *server_pointer;
	int return_value;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (return_value = getaddrinfo(server_ip, server_port, &hints, &server_info) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %d\n", return_value);
	}

	for (server_pointer = server_info; server_pointer != NULL; server_pointer->ai_next)
	{
		if ((sockfd = socket(server_pointer->ai_family, server_pointer->ai_socktype, server_pointer->ai_protocol)) == -1)
		{
			printf("socket connected");
			continue;
		}
		if (connect(sockfd, server_pointer->ai_addr, server_pointer->ai_addrlen) == -1)
		{
			close(sockfd);
			printf("client not connected");
			continue;
		}
		else
		{
			break;
		}
	}

	Message login_mes;
	login_mes.type = LOGIN;
	strcpy(login_mes.source, client_id);
	strcpy(login_mes.data, password);
	
	login_mes.size = strlen(password);
	strcpy(buffer, serialize(login_mes));
	if (!send_buffer())
	{
		fprintf(stderr, "Couldn't send login info\n");
		return;
	}

	int num_bytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
	if (num_bytes == -1)
	{
		fprintf(stderr, "Failed to receive");
		close(sockfd);
		return;
	}
    
	Message *response = deserialize(buffer);
	clear_buffer();
	if (response->type == LO_ACK)
	{
		fprintf(stderr, "Logged in successfully!\n");
		logged_in = true;
		return;
	}
	else if (response->type == LO_NAK)
	{
		fprintf(stderr, "Login failed: %s\n", response->data);
		close(sockfd);
		return;
	}
	else
	{
		fprintf(stderr, "very big wrong ahhh!");
	}
}

void logout()
{
	Message logout_mes;
	logout_mes.type = EXIT;
	logout_mes.size = strlen("blank");
	strcpy(logout_mes.source, client_id);
	strcpy(logout_mes.data, "blank");

	char *mes_string = serialize(logout_mes);

	strcpy(buffer, mes_string);
	send_buffer();

	close(sockfd);
	logged_in = false;
	printf("You are no longer logged in\n");
	return;
}

void joinsession(char *session_id)
{

	Message join_mes;
	join_mes.type = JOIN;
	strcpy(join_mes.data, session_id);
	join_mes.size = strlen(session_id);
	strcpy(join_mes.source, client_id);

	char *join_string = serialize(join_mes);
	int num_bytes;
	strcpy(buffer, join_string);
	send_buffer();

	// check the type of ACK (JN_ACK or JN_NAK) for join and handle appropriately
	if ((num_bytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1)
	{
		printf("failed recieve");
		close(sockfd);
		return;
	}
	fprintf(stderr, "Buffer received %s %d\n", buffer, num_bytes);
	Message *response = deserialize(buffer);
	clear_buffer();

	if (response->type == JN_ACK)
	{
		fprintf(stderr, "Joined session %s successfully\n", session_id);
		in_session = true;
		return;
	}
	else if (response->type == JN_NAK)
	{
		printf("%s", response->data);
		close(sockfd);
		return;
	}
}

void createsession(char *session_id)
{

	printf("Creating session: %s \n", session_id);

	Message create_mes;
	create_mes.type = NEW_SESS;
	strcpy(create_mes.data, session_id);
	create_mes.size = strlen(session_id);
	strcpy(create_mes.source, client_id);

	char *create_string = serialize(create_mes);
	strcpy(buffer, create_string);

	if (!send_buffer())
	{
		fprintf(stderr, "Couldn't send create info\n");
		return;
	}

	int num_bytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
	if (num_bytes == -1)
	{
		fprintf(stderr, "Failed to receive");
		close(sockfd);
		return;
	}

	Message *response = deserialize(buffer);
	clear_buffer();
	if (response->type == NS_ACK)
	{
		fprintf(stderr, "Session %s created\n", session_id);
		in_session = true;
		return;
	}
	else if (response->type == NS_NAK)
	{
		printf("%s", response->data);
		in_session = false;
		return;
	}
	else
	{
		printf("very big wrong ahhh!");
	}

	return;
}

void leavesession()
{

	if (!in_session)
	{
		printf("You are not in a session.\n");
		return;
	}

	Message leave_mes;
	leave_mes.type = LEAVE_SESS;
	leave_mes.size = 0;
	strcpy(leave_mes.source, client_id);
	strcpy(leave_mes.data, " ");

	char *leave_string = serialize(leave_mes);
	int num_bytes;
	strcpy(buffer, leave_string);

	if (send_buffer())
	{
		printf("Leaving session\n");
		in_session = false;
		return;
	}
	printf("Couldnt leave session; This shouldn't happen... Quitting\n");
	exit(1);
}

void printlist(char *string)
{
	char s[MAX_DATA];
	strcpy(s, string);
	char *p = strtok(s, "-");
	printf("No session:\n", s);
	int sess_name = 0;
	while (p != NULL)
	{	
		// Check if hitting a new session in the list
		if (strcmp(p, "session") == 0) sess_name = 1;

		// Currently on the session name
		else if(sess_name){ printf("\nIn session %s\n",p); sess_name--;}

		// On a name; Just print it
		else printf("%s\n", p);
		
		p = strtok(NULL, "-");
	}
	return;
}

void list()
{
	// if (!in_session)
	// {
	// 	fprintf(stderr, "You are not in a session.\n");
	// 	return;
	// }

	printf("printing list:\n");

	Message list_mes;
	list_mes.type = QUERY;
	list_mes.size = 0;
	strcpy(list_mes.source, client_id);

	char *list_string = serialize(list_mes);
	strcpy(buffer, list_string);

	send_buffer();
    
	if ((num_bytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1)
	{
		printf("failed recieve");
		close(sockfd);
		return;
	}
	fprintf(stderr, "hmm %s %d\n", buffer, num_bytes);
	Message *response = deserialize(buffer);
	clear_buffer();

	if (response->type == QU_ACK)
	{
		printlist(response->data);
		return;
	}
	else
	{
		printf("Error in receiving list\n");
		return;
	}
}

void send_text(char *text)
{
	int numbytes;
	Message text_mes;
	text_mes.type = MESSAGE;
	strcpy(text_mes.source, client_id);

	strcpy(text_mes.data, text);
	text_mes.size = strlen(text);
	char *text_string = serialize(text_mes);

	strcpy(buffer, text_string);

	send_buffer();
	return;
}

void transferuser(char *kick_id)
{
	// check for possible errors
	Message kick_mes;
	kick_mes.type = ADM_KICK;
	strcpy(kick_mes.source, client_id);
	strcpy(kick_mes.data, kick_id);
	kick_mes.size = strlen(kick_id);
	strcpy(buffer, serialize(kick_mes));

	printf("%s\n", buffer);
	if (!send_buffer())
	{
		fprintf(stderr, "Couldn't send kick info\n");
		return;
	}

	int num_bytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
	if (num_bytes == -1)
	{
		fprintf(stderr, "Failed to receive");
		return;
	}

	Message *response = deserialize(buffer);
	clear_buffer();
	if (response->type == ADM_ACK)
	{
		fprintf(stderr, "Client %s was kicked!\n", kick_id);
		return;
	}
	else if (response->type == ADM_NAK)
	{
		fprintf(stderr, "%s", response->data);
		return;
	}
	else
	{
		fprintf(stderr, "very big wrong ahhh!");
	}
}


void kickuser(char *tran_id)
{
	// check for possible errors
	Message tran_mes;
	tran_mes.type = ADM_KICK;
	strcpy(tran_mes.source, client_id);
	strcpy(tran_mes.data, tran_id);
	tran_mes.size = strlen(tran_id);
	strcpy(buffer, serialize(tran_mes));

	printf("%s\n", buffer);
	if (!send_buffer())
	{
		fprintf(stderr, "Couldn't send transfer info\n");
		return;
	}

	int num_bytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
	if (num_bytes == -1)
	{
		fprintf(stderr, "Failed to receive");
		return;
	}

	Message *response = deserialize(buffer);
	clear_buffer();
	if (response->type == ADM_ACK)
	{
		fprintf(stderr, "Client %s is now the admin!\n", tran_id);
		return;
	}
	else if (response->type == ADM_NAK)
	{
		fprintf(stderr, "%s", response->data);
		return;
	}
	else
	{
		fprintf(stderr, "very big wrong ahhh!");
	}
}

int main()
{
	// input strings
	char cmd[COMMAND_LEN];
	char session_id[COMMAND_LEN];
	char password[COMMAND_LEN];
	char server_ip[COMMAND_LEN];
	char server_port[COMMAND_LEN];

	// socket
	int num_bytes;
	socklen_t servaddr_len;
	struct sockaddr_in servaddr;

	// status
	logged_in = false;

	fd_set socketset;

	while (1)
	{

		FD_ZERO(&socketset);
		FD_SET(fileno(stdin), &socketset);

		if (sockfd > 0)
		{

			FD_SET(fileno(stdin), &socketset);
			select(fileno(stdin) + 1, &socketset, NULL, NULL, NULL);
			// TODO: it keeps looping here if i dont comment the below out
			// FD_SET(sockfd, &socketset);
			// select(sockfd + 1, &socketset, NULL, NULL, NULL);
		}
		else
		{

			select(fileno(stdin) + 1, &socketset, NULL, NULL, NULL);
		}

		// Receive message
		if (logged_in && FD_ISSET(sockfd, &socketset) && in_session)
		{
			char buf[MAX_DATA];
			recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
			Message *response = deserialize(buffer);
			clear_buffer();
			if (response->type == MESSAGE)
			{
				printf("%s", response->data);
			}
		}
		else if (FD_ISSET(fileno(stdin), &socketset))
		{
			scanf("%s", cmd);

			if (strcmp(cmd, "/login") == 0)
			{   
				if (logged_in)
				{
					char *garb;
					scanf("%s", garb);
					scanf("%s", garb);
					scanf("%s", garb);
					scanf("%s", garb);
					printf("Already logged in\n");
				}
				else
				{
					scanf("%s", client_id);
					scanf("%s", password);
					scanf("%s", server_ip);
					scanf("%s", server_port);
					login(client_id, password, server_ip, server_port);
				}
			}
			else if (strcmp(cmd, "/logout") == 0)
			{
				if (!logged_in)
				{
					printf("currently not logged in\n");
				}
				else
				{
					logout();
				}
			}
			else if (strcmp(cmd, "/joinsession") == 0)
			{
				scanf("%s", session_id);
				if (!logged_in)
				{
					printf("please log in first\n");
				}
				else if (in_session)
				{
					printf("already in a session\n");
				}
				else
				{
					joinsession(session_id);
				}
			}
			else if (strcmp(cmd, "/leavesession") == 0)
			{
				if (!logged_in)
				{
					printf("please log in first\n");
				}
				else if (!in_session)
				{
					printf("not in a session\n");
				}
				else
				{
					leavesession();
				}
			}
			else if (strcmp(cmd, "/createsession") == 0)
			{
				scanf("%s", session_id);
				if (!logged_in)
				{
					printf("please log in first\n");
				}
				else
				{
					createsession(session_id);
				}
			}
			else if (strcmp(cmd, "/list") == 0)
			{
				if (!logged_in)
				{
					printf("please log in first\n");
				}
				else
				{
					list();
				}
			}
			else if (strcmp(cmd, "/transfer") == 0)
			{
				scanf("%s", client_id);
				if (!logged_in)
				{
					printf("please log in first\n");
				}
				else if (!in_session)
				{
					printf("please create a session first\n");
				}
				else
				{
					transferuser(client_id);
				}
			}
			else if (strcmp(cmd, "/quit") == 0)
			{
				if (logged_in)
				{
					logout();
				}
				return 0;
			}
			else
			{
				// send the text if logged in and in a session
				if (logged_in)
				{
					if (in_session)
					{
						char totaltext[MAX_DATA];
						strcpy(totaltext, cmd);
						int cmdlen = strlen(cmd);
						fgets(totaltext + cmdlen, MAX_DATA - cmdlen, stdin);
						printf("sending message: %s", totaltext);
						send_text(totaltext);
					}
					else
					{
						fprintf(stderr, "You are not currently in a session\n");
					}
				}
				else
				{
					fprintf(stderr, "Please log in first\n");
				}
			}
		}
	}

	return 0;
}
