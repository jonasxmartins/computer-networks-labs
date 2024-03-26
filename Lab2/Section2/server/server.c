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
#include <time.h>
#include <errno.h>

#define TIMEOUT 300

// key variables for server functionality
unsigned int n_sessions = 0; // NOT HARD CODED
unsigned int n_clients = 0; // HARD CODED
struct Session session_list[16]; // NOT HARD CODED
struct Client client_list[64]; // PARTIALLY HARD CODED
int current_sock = 0;
int client_init(char name[64], char password[64]);
int initialize_database();

void *client_handler(void *args) 
{

    int sock = current_sock;

    char buffer[BUFFERSIZE];
    struct Packet login_packet;
    bool login_success = false;
    struct Client *self;
    
    // Loop until login is successful
    while (!login_success) 
    {
        memset(buffer, 0, BUFFERSIZE);
        
        time_t start = time(NULL);

        while(1)
       {
            ssize_t received_len = recv(sock, buffer, BUFFERSIZE - 1, MSG_DONTWAIT);
            if (received_len > 0)
            {
                break;
            }
            else if (received_len == 0) 
            {
                printf("Client disconnected\n");
            } 
            else if (received_len == -1 && errno == EAGAIN || errno == EWOULDBLOCK)
            {
                time_t now = time(NULL);
                if (now - start >= TIMEOUT)
                {
                    struct Packet pack;
                    attempt_leave(self, pack, session_list, n_sessions, sock);
                    close(sock);
                    return NULL;
                }
            }
            else 
            {
                printf("Client disconnected\n");
                close(sock);
                return NULL;
            }

        }
        
        login_packet = message_to_packet(buffer);
        int login_result = attempt_login(sock, login_packet, client_list, n_clients);

        if (login_result == 0) {
            printf("client_handler: login successful.\n");
            login_success = true;
            // Find the client object corresponding to the logged-in client
            for (int i = 0; i < n_clients; i++) {
                if (strcmp(login_packet.source, client_list[i].client_id) == 0) {
                    self = &client_list[i];
                    break;
                }
            }
            if (!self) {
                printf("Error: Client not found in the client list after login.\n");
                close(sock);
                return NULL;
            }
        } 
        else if (login_result != -3){
            int registration_success = client_init(login_packet.source, login_packet.data);
            if (registration_success == 1){
                printf("client_handler: created new user.\n");
                self = &client_list[n_clients-1];
                if (!self) {
                    printf("Error: Client not found in the client list after login.\n");
                    close(sock);
                    return NULL;
                }
                self->connected = 1;
                login_success = true;
                printf("client_handler: created user and login successful.");
            }
            else
            {
                printf("Login failed\n"); 
            }
        }
    }

    // Main command processing loop
    while (login_success && self && self->connected) 
    {

        memset(buffer, 0, BUFFERSIZE);

        ssize_t received_len = recv(sock, buffer, BUFFERSIZE, 0);
        
        if (received_len <= 0) 
        {
            if (received_len == 0) 
            {
                printf("Client disconnected\n");
            } 
            else 
            {
                perror("Client not connected");
            }
            break;
        }

        struct Packet req_packet = message_to_packet(buffer);
        switch (req_packet.type) 
        {
            case LOGIN:
            {
            // already logged in
                struct Packet resp_packet;
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
            }
            case EXIT:
            {
                self->connected = 0;
                self->in_session = 0;  
                break;
            }
            case JOIN:
            {
                printf("client_handler: attempting to join session.\n");
                if (attempt_join(self, req_packet, session_list, n_sessions, sock) >= 0)
                    printf("Joining session successful\n");
                else printf("Joining session unsuccessful\n");
                break;
            }
            case LEAVE_SESS:
            {
                printf("client_handler: attempting to leave session.\n");
                if (attempt_leave(self, req_packet, session_list, n_sessions, sock) >= 0)
                    printf("Leaving session successful\n");
                else printf("Leaving session unsuccessful\n");
                break;
            }
            case NEW_SESS:
            {
                printf("client_handler: attempting to create session.\n");
                int res = attempt_new(self, req_packet, session_list, n_sessions, sock);
                printf("res%d\n", res);
                if (res >= 0) {
                    printf("New session successful\n");
                    printf("client_handler: first session created.\n"
                            "User name: %d\n"
                            "User session_id: %d\n"
                            "session_list[0]: %d\n"
                            "session_list[0].clients_in_list[0]: %d\n", self->client_id, self->session_id, session_list[0].session_id, session_list[0].clients_in_list[0]->client_id);
                }
                else printf("New session unsuccessful\n");
                break;
            }
            case MESSAGE:
            {
                send_message(self, req_packet, session_list, n_sessions, sock);
                break;
            }
            case QUERY:
            {
                if (send_query_message(self, client_list, session_list, n_sessions, n_clients, sock) >= 0)
                    printf("Query sent successfully\n");
                else printf("Query sent unsuccessfully\n");
                break;
            }
            default:
                break;
        }
    }

    if (self) 
    {
        self->connected = 0;
        self->in_session = 0;
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

    printf("Server is up and running\n");
    initialize_database();

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }
        printf("main_thread\n");
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

int client_init(char name[64], char password[64])
{
    if (n_clients == 64)
    {
        printf("client limit reached\n");
        return -1;
    }
    struct Client user;
    strncpy(user.client_id, name, MAX_NAME);
    strncpy(user.password, password, MAX_NAME);
    user.connected = 0;
    user.in_session = 0;
    user.socket_fd = 0;

    client_list[n_clients] = user;

    n_clients++;

    FILE *file = fopen("./server/users.csv", "a");
    if (file == NULL) {
        return -1; 
    }

    fprintf(file, "\n%s,\"%s\"", name, password); 
    fclose(file);

    return 1;

}

int initialize_database()
{
    FILE *file = fopen("./server/users.csv", "r");
    if (file == NULL) {
        printf("Failed to open users.csv\n");
        return -1;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {

        char name[64];
        char password[64];

        if (sscanf(line, "%[^,],\"%[^\"]\"", name, password) == 2) {
            if (n_clients >= 64) {
                printf("Client limit reached during database initialization\n");
                fclose(file);
                return -1;
            }

            struct Client user;
            strncpy(user.client_id, name, MAX_NAME);
            strncpy(user.password, password, MAX_NAME);
            user.connected = 0;
            user.in_session = 0;
            user.socket_fd = 0;

            client_list[n_clients] = user;
            n_clients++;
        } else {
            printf("Failed to parse line: %s\n", line);
        }
    }

    fclose(file);
    return 0; // Success
}
