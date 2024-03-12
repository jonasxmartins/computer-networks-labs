#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include "structs/structs.h"
#include <unistd.h>

#define BUFFERSIZE 2048

void *log_in(void *arg);
void *server_thread(void *arg);
void *user_thread(void *arg);

void nack_join(struct Packet pack);
void ack_query(struct Packet pack);
int command_encoder(char* command);
void request_command(int command);
void create_or_join_session(char *buffer, int command);
void send_message(char *buffer);

//Global variables
bool in_session = false;
char* client_id;

int fd;

int main(void)
{
    log_in(NULL);
    pthread_exit(0);
}

void *log_in(void *arg)
{
    int error;
    struct addrinfo info, *serverInfo, *final;
    char buffer[BUFFERSIZE];

    printf("Please login\n");
    fgets(buffer, sizeof(buffer), stdin);

    buffer[strcspn(buffer, "\n")] = '\0';
    char* command = strtok(buffer, " ");
    client_id = strtok(NULL, " ");
    char* password = strtok(NULL, " ");
    char* server_IP = strtok(NULL, " ");
    char* server_port = strtok(NULL, " ");

    // initializes addrinfo struct that will be used for getaddrinfo
    memset(&info, 0, sizeof(info));
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_STREAM;

    // gets a linked list of all the possible address that could work
    error = getaddrinfo(server_IP, server_port, &info, &serverInfo);
    if (error != 0)
    {
        printf("error: %s\n", gai_strerror(error));
        exit(99);
    }

    // iterates over all the addresses obtained until finding one that works
    for (final = serverInfo; final != NULL; final = final->ai_next)
    {
        fd = socket(final->ai_family, final->ai_socktype, 0);
        if (fd == -1)
        {
            perror("client socket");
            continue;
        }

        if (connect(fd, final->ai_addr, final->ai_addrlen) == -1)
        {
            perror("connect");
            continue;
        }

        break;

    }

    if (final == NULL)
    {
        fprintf(stderr, "Error: couldn't connect to server");
        exit(8);
    }

    bool logged_in = false;

    // make sure user is properly logged in
    while(!logged_in)
    {
        //send login info
        char message[BUFFERSIZE];
        struct Packet pack = make_packet(LOGIN, strlen(password), client_id, password);
        packet_to_message(pack, message);

        if (!send(fd, message, strlen(message), 0))
        {
            perror("send login");
            exit(2);
        }
        
        if (!recv(fd, buffer, BUFFERSIZE, 0))
        {
            perror("recv ack login");
            exit(3);
        }

        struct Packet ans = message_to_packet(buffer);

        if (ans.type == 2)
        {
            printf("You are logged in!\n");
            logged_in = true;
        }
        else
        {
            printf("Please enter valid credentials to login\n");
            fgets(buffer, sizeof(buffer), stdin);

            char* client_id = strtok(NULL, " ");
            char* password = strtok(NULL, " ");
        }
    }
    
    //create threads
    pthread_t wait_server, wait_user;

    if (pthread_create(&wait_server, NULL, server_thread, NULL) != 0)
    {
        perror("server thread");
        exit(99);
    }

    if (pthread_create(&wait_user, NULL, user_thread, NULL) != 0)
    {
        perror("user thread");
        exit(99);
    }
}

void *server_thread(void *arg)
{
    while(1)
    {
        char buffer[BUFFERSIZE];

        int rl = recv(fd, buffer, BUFFERSIZE, 0);

        if (rl <= 0)
        {
            if (rl == 0)
            {
                printf("Server closed the connection\n");
                break;
            }
            else
            {
                perror("server thread recv");
                exit(99);
            }
        }

        buffer[rl] = '\0';

        struct Packet received = message_to_packet(buffer);

        switch (received.type)
        {
        case 6:
            printf("Joined session %s succesfully\n", received.data);
            in_session = 1;
            break;
        case 7:
            nack_join(received);
            break;
        case 10:
            printf("Created session %s succesfully\n", received.data);
            in_session = 1;
            break;
        case 11:
            printf("%s says:\n%s\n", received.source, received.data);
            break;
        case 13:
            ack_query(received);
            break;
        default:
            printf("Unrecognized message from the server with code %d\n", received.type);
            break;
        }  
    }
}







void nack_join(struct Packet pack)
{
    char data[MAX_DATA];

    strncpy(data, pack.data, MAX_DATA);

    char* session = strtok(data, ", ");
    char* reason = strtok(NULL, ", ");

    printf("Unable to join session %s, reason:\n%s\n", session, reason);
}

void ack_query(struct Packet pack)
{
    return;
}


void *user_thread(void *arg)
{
    bool connection_active = true;
    
    while (connection_active)
    {
        char buffer[BUFFERSIZE];

        printf("Enter a command\n");

        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        char *command = strtok(buffer, " ");
          

        int type = command_encoder(command);
        if (type == 0)
        {
            continue;
        }

        switch (type)
        {
        case EXIT:
        {
            request_command(EXIT);
            break;
        }
        case LEAVE_SESS:
        {
            request_command(LEAVE_SESS);
            break;
        }
        case QUERY:
        {
            request_command(QUERY);
            break;
        }
        case JOIN:
        {
            char *session = strtok(NULL, " ");
            if (session == NULL) printf("Invalid session name\n");
            else create_or_join_session(buffer, JOIN);
            break;
        }
        case NEW_SESS:
        {
            char *session = strtok(NULL, " ");
            if (session == NULL) printf("Invalid session name\n");
            else create_or_join_session(buffer, NEW_SESS);
            break;
        }
        case MESSAGE:
        {
            int sz = strlen(buffer);
            buffer[sz] = ' ';
            send_message(buffer);
        }
        default:
            break;
        }
    }
}

int command_encoder(char* command)
{
    if (strcmp(command, "/login") == 0)
    {
        return LOGIN;
    }
    if (strcmp(command, "/exit") == 0)
    {
        return EXIT;
    }
    if (strcmp(command, "/leave_sess") == 0)
    {
        return LEAVE_SESS;
    }
    if (strcmp(command, "/join_sess") == 0)
    {
        return JOIN;
    }
    if (strcmp(command, "/new_sess") == 0)
    {
        return NEW_SESS;
    }
    if (strcmp(command, "/query") == 0)
    {
        return QUERY;
    }
    if (in_session && command[0] != '/')
    {
        return MESSAGE;
    }

    printf("ERROR: Unknown command\n");
    return 0;

}

void request_command(int command)
{
    char message[BUFFERSIZE];
    struct Packet pack = make_packet(command, 8, client_id, "command\n");
    packet_to_message(pack, message);
    if (!send(fd, message, strlen(message), 0))
        {
            perror("send request");
            exit(2);
        }
}

void create_or_join_session(char *buffer, int command)
{
    char message[BUFFERSIZE];
    struct Packet pack = make_packet(command, strlen(buffer), client_id, buffer);
    packet_to_message(pack, message);
    if (!send(fd, message, strlen(message), 0))
        {
            perror("send session");
            exit(2);
        }
}

void send_message(char *buffer)
{
    char message[BUFFERSIZE];
    struct Packet pack = make_packet(MESSAGE, strlen(buffer), client_id, buffer);
    packet_to_message(pack, message);
    if (!send(fd, message, strlen(message), 0))
        {
            perror("send message");
            exit(2);
        }
}