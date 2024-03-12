#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>

#define BUFFERSIZE 2048

void server_thread();
void user_thread();

int main(void)
{
    int fd;
    int error;
    struct addrinfo info, *serverInfo, *final;
    char buffer[BUFFERSIZE];

    printf("Please login");
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

    send(fd, )

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

    return 0;
}