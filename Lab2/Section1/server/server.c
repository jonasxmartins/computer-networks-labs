#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>


void *client_handler(void *arg) {
    int sock = *(int *)arg;
    free(arg); // Free the heap memory allocated for the socket descriptor

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
        if (pthread_create(&tid, NULL, client_handler, (void *)new_sock) < 0) {
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