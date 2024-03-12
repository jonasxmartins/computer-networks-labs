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


// key variables for server functionality
unsigned int n_sessions = 0; // NOT HARD CODED
unsigned int n_clients = 1; // HARD CODED
struct Session session_list[16]; // NOT HARD CODED
struct Client client_list[64]; // PARTIALLY HARD CODED
int current_sock = 0;

void *client_handler(void *args) {

    int sock = current_sock;

    char buffer[BUFFERSIZE];
    struct Packet login_packet;
    bool login_success = false;
    struct Client *self;
    self->in_session = 0;
    self->socket_fd = sock;
    self->connected = 0;

    // Loop until login is successful
    while (!login_success) {
        memset(buffer, 0, BUFFERSIZE);

        ssize_t received_len = recv(sock, buffer, BUFFERSIZE - 1, 0);
        if (received_len < 0) {
            printf("Client not connected\n");
            break;
        } else if (received_len == 0) {
            printf("Client disconnected\n");
            break; 
        }
        login_packet = message_to_packet(buffer);

        if (attempt_login(sock, login_packet, client_list, n_clients) >= 0) {
            printf("Login successful\n");
            login_success = true;
            for (int i = 0; i < n_clients; i++) {
                if (strcmp(login_packet.source, client_list[i].client_id) == 0){
                    self = &client_list[i];
                    break;
                }
            } 
        } else {
            printf("Login failed");
        }
    }

    // You can use a loop here to continuously read from the socket
    // and process commands from the client based on your protocol
    while(1){
        memset(buffer, 0, BUFFERSIZE);
        struct Packet req_packet, resp_packet;

        ssize_t received_len = recv(sock, buffer, BUFFERSIZE - 1, 0);
        if (received_len < 0) {
            printf("Client not connected\n");
            break;
        } else if (received_len == 0) {
            printf("Client disconnected\n");
            break; 
        }
        req_packet = message_to_packet(buffer);
        int req_type = req_packet.type;

        switch(req_type) {
            case LOGIN:
            // already logged in
                resp_packet.type = LO_ACK;
                strcpy((char *)resp_packet.source, "SERVER");
                strcpy((char *)resp_packet.data, "You are already logged in!.");
                resp_packet.size = strlen(resp_packet.data);

                char response_buffer[BUFFERSIZE];
                memset(response_buffer, 0, BUFFERSIZE);

                packet_to_message(resp_packet, response_buffer);
                response_buffer[strlen(response_buffer)] = '\0';
                if (send(sock, response_buffer, strlen(response_buffer), 0) < 0) {
                    perror("send failed");
                    exit(-1);
                }
                break;

            case EXIT:
                self->connected = 0;
                self->in_session = 0;  
                break;

            case JOIN:
                if (attempt_join(self, req_packet, session_list, n_sessions) >= 0)
                    printf("Joining session successful\n");
                else printf("Joining session unsuccessful\n");
                break;

            case LEAVE_SESS:
                if (attempt_leave(self, req_packet, session_list, n_sessions) >= 0)
                    printf("Leaving session successful\n");
                else printf("Leaving session unsuccessful\n");
                break;

            case NEW_SESS:
                if (attempt_new(self, req_packet, session_list, n_sessions) >= 0)
                    printf("New session successful\n");
                else printf("New session unsuccessful\n");
                break;

            case MESSAGE:
                send_message(self, req_packet, session_list, n_sessions);
                break;

            case QUERY:
                if (send_query_message(self, client_list, session_list, n_sessions, n_clients) >= 0)
                    printf("Query sent successfully\n");
                else printf("Query sent unsuccessfully\n");
                break;

            default:
                break;
        }
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

    struct Client mario;
    strncpy(mario.client_id, "mario\0", 6);
    strncpy(mario.password, "hello\0", 6);
    mario.connected = 0;
    mario.in_session = 0;
    mario.socket_fd = 0;

    client_list[0] = mario;


    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    printf("Server is up and running\n");

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        // Create a new thread for this client
        pthread_t tid;
        current_sock = newsockfd;

        if (pthread_create(&tid, NULL, client_handler, 0) < 0) {
            perror("ERROR creating thread");
            close(newsockfd);
            continue;
        }

        // Detach the thread - let it clean up itself after finishing
        pthread_detach(tid);
    }

    // Close the server socket and clean up (not reached in this example)
    close(sockfd);
    return 0;
}