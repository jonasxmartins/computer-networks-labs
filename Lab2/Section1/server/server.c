#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>

struct thread_args {
    int new_sock;
    unsigned int *n_sessions;
    unsigned int *n_clients;
    struct Client *client_list;
    struct Session *session_list;
};


void *client_handler(void *args) {
    int sock = *(int *)args[0];
    unsigned int *n_sessions = args[1];
    unsigned int *n_clients = args[2];
    struct Client *client_list = args[3];
    struct Session *session_list = (struct Session *) args[4];
    
    free(args); // Free the heap memory allocated for the socket descriptor

    // You can use a loop here to continuously read from the socket
    // and process commands from the client based on your protocol
    while(1){
        // handles requests etc. 
    }

    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    // need to implement session, session lists, client, client lists etc
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    // key variables for server functionality
    unsigned int n_sessions = 0;
    unsigned int n_clients = 0;
    struct Session *session_list[16];
    struct Client *client_list[64];

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        // Allocate memory for a new socket descriptor
        int *new_sock = malloc(sizeof(int));
        if (new_sock == NULL) {
            perror("ERROR allocating memory for new socket");
            close(newsockfd);
            continue;
        }
        *new_sock = newsockfd;

        // Create a new thread for this client
        pthread_t tid;

        struct thread_args *args = malloc(sizeof(struct thread_args));
        if (args == NULL) {
            perror("ERROR allocating thread arguments");
        }
        args->new_sock = new_sock;
        args->client_list = &client_list;
        args->session_list = &session_list;
        args->n_clients = &n_clients;
        args->n_sessions = &n_sessions;

        if (pthread_create(&tid, NULL, client_handler, (void *)args) < 0) {
            perror("ERROR creating thread");
            close(newsockfd);
            free(new_sock);
            continue;
        }

        // Detach the thread - let it clean up itself after finishing
        pthread_detach(tid);
    }

    // Close the server socket and clean up (not reached in this example)
    close(sockfd);
    return 0;
}