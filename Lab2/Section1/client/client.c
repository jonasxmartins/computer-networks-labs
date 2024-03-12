#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include "structs/structs.h"

#define BUFFERSIZE 2048

void log_in();
void server_thread();
void user_thread();

void nack_join(struct Packet pack);
void ack_query(struct Packet pack);

int fd;

int main(void)
{
    log_in();

    return 0;
}

void log_in()
{
    int fd;
    int error;
    struct addrinfo info, *serverInfo, *final;
    char buffer[BUFFERSIZE];

    printf("Please login\n");
    fgets(buffer, sizeof(buffer), stdin);

    buffer[strcspn(buffer, "\n")] = '\0';
    char* command = strtok(buffer, " ");
    char* client_id = strtok(NULL, " ");
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
            close(fd);
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
        struct Packet pack = make_packet(1, strlen(password), client_id, password);
        packet_to_message(pack, message);
        if (!send(fd, message, strlen(message), NULL))
        {
            perror("send login");
            exit(2);
        }

        if (!recv(fd, buffer, BUFFERSIZE, NULL))
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

void server_thread()
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
                perror("server thread");
                exit(99);
            }
        }

        buffer[rl] = '\0';

        struct Packet received = message_to_packet(buffer);

        switch (received.type)
        {
        case 6:
            printf("Joined session %s succesfully\n", received.data);
            break;
        case 7:
            nack_join(received);
            break;
        case 10:
            printf("Created session %s succesfully\n", received.data);
            break;
        case 11:
            printf("%s says:\n%s\n", received.source, received.data);
            break;
        case 13:
            ack_query(received);
            break;
        default:
            printf("Unrecognized message from the server with code %d", received.type);
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