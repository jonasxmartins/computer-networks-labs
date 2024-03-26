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


int attempt_login(int sock, struct Packet packet, struct Client *client_list, int n_clients);
int attempt_join(struct Client *self, struct Packet packet, int sock);
int attempt_leave(struct Client *self, struct Packet packet, int sock);
int attempt_new(struct Client *self, struct Packet packet, int sock);
int send_query_message(struct Client *self, int sock);
void send_message(struct Client *self, struct Packet packet, int sock);
void print_packet(struct Packet pack);
void print_client(struct Client client);

// key variables for server functionality
unsigned int n_sessions = 0; 
unsigned int n_clients = 0; 
struct Session session_list[16]; 
struct Client client_list[64]; 
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

        while(1)
        {
            ssize_t received_len = recv(sock, buffer, BUFFERSIZE - 1, 0);
            if (received_len > 0)
            {
                break;
            }
            else if (received_len == 0) 
            {
                printf("Client disconnected\n");
            } 
            else if (received_len == -1)
            {
                printf("error logging in\n");
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
                    self->socket_fd = sock;
                    break;
                }
            }
            if (!self) {
                printf("Error: Client not found in the client list after login.\n");
                close(sock);
                return NULL;
            }
        } 
        else if (login_result == -3){
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
                printf("client_handler: created user and login successful.\n");
            }
            else
            {
                printf("Login failed\n"); 
                close(sock);
                return NULL;
            }
        }
    }
    self->socket_fd = sock;

    // Main command processing loop
    while (login_success && self && self->connected) 
    {

        memset(buffer, 0, BUFFERSIZE);

        time_t start = time(NULL);

        while(1)
        {
            ssize_t received_len = recv(sock, buffer, BUFFERSIZE, MSG_DONTWAIT);
            if (received_len <= 0) 
            {
                if (received_len == 0) 
                {
                    printf("Client disconnected\n");
                    close(sock);
                    return NULL;
                } 
                else if (received_len == -1) 
                {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) 
                    {
                        time_t now = time(NULL);
                        if (now - start > TIMEOUT) {
                            printf("Connection timed out for %s\n", self->client_id);
                            close(sock);
                            return NULL;
                        }
                    } 
                    else 
                    {
                        perror("recv error");
                        close(sock);
                        return NULL;
                    }
                }
            }
            else
            {
                break;
            }
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
                if (attempt_join(self, req_packet, sock) >= 0)
                    printf("Joining session successful\n");
                else printf("Joining session unsuccessful\n");
                break;
            }
            case LEAVE_SESS:
            {
                printf("client_handler: attempting to leave session.\n");
                if (attempt_leave(self, req_packet, sock) >= 0)
                    printf("Leaving session successful\n");
                else printf("Leaving session unsuccessful\n");
                break;
            }
            case NEW_SESS:
            {
                printf("client_handler: attempting to create session.\n");
                int res = attempt_new(self, req_packet, sock);
                if (res >= 0) {
                    printf("New session successful\n");
                }
                else printf("New session unsuccessful\n");
                break;
            }
            case MESSAGE:
            {
                send_message(self, req_packet, sock);
                break;
            }
            case QUERY:
            {
                if (send_query_message(self, sock) >= 0)
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


int attempt_login(int sock, struct Packet packet, struct Client *client_list, int n_clients) {
    int login_status = -1;

    for (int i = 0; i < n_clients; i++) 
    {
        if (strcmp(packet.source, client_list[i].client_id) == 0) 
        {
            if (client_list[i].connected == 0)
            {
                if (strcmp(packet.data, client_list[i].password) != 0) 
                { 
                    printf("There was a problem in connecting the client. Wrong password\n");
                    break;
                }
                else 
                {
                    printf("attempt_login: client %s has been connected.", client_list[i].client_id);
                    client_list[i].connected = 1;
                    login_status = 0; // Login success
                    break;
                }
            }
            else
            {
                printf("There was a problem in connecting the client. Client not connected\n");
                break;
            }
        }
        else
        {
            login_status = -3;
        }
    }

    if (login_status == -1){
        printf("There was a problem in connecting the client.\n"); 
    }

    // Prepare a response packet
    struct Packet response_packet;
    memset(&response_packet, 0, sizeof(response_packet)); // Clear the response packet structure

    if (login_status == 0 || login_status == -3) 
    {
        // Login successful, prepare lo_ack response
        response_packet.type = LO_ACK;
        strcpy((char *)response_packet.source, "SERVER");
        strcpy((char *)response_packet.data, "Login Successful.");
        response_packet.size = 18;

    } 
    else 
    {
        // Login failed, prepare lo_nak response
        response_packet.type = LO_NAK;
        strcpy((char *)response_packet.source, "SERVER");
        strcpy((char *)response_packet.data, "Login Unsuccessful.");
        response_packet.size = 20;
    }
    // Convert response_packet to a string

    char response_buffer[BUFFERSIZE];
    memset(response_buffer, 0, BUFFERSIZE);
    packet_to_message(response_packet, response_buffer);

    // Send the response
    if (send(sock, response_buffer, strlen(response_buffer), 0) < 0) 
    {
        perror("Failed to send attempt_login ACK or NAK message.");
        return -1;
    }
    return login_status;
}

int attempt_join(struct Client *self, struct Packet packet, int sock){
    if (n_sessions == 0){
        printf("attempt_join: there are no sessions to join.\n");
    }
    int join_status = -1;
    int sess_index = 0;
    bool session_exists = false;
    for (int i = 0; i < n_sessions; i++){
        if (strcmp(packet.data, session_list[i].session_id) == 0) {
            session_exists = true;
            sess_index = i;
            break;
        }
    }
    if (self->connected == 1 && self->in_session == 0 && session_exists) join_status = 0;
    
    if (join_status == 0){
        memcpy(self->session_id, packet.data, packet.size);
        self->in_session = 1;
        int n_cli = session_list[sess_index].n_clients_in_sess;
        session_list[sess_index].clients_in_list[n_cli] = self; 
        session_list[sess_index].n_clients_in_sess++;
    }
    struct Packet response_packet;
    memset(&response_packet, 0, sizeof(response_packet)); // Clear the response packet structure

    if (join_status == 0) {
        // Login successful, prepare lo_ack response
        response_packet.type = JN_ACK;
        strcpy((char *)response_packet.source, "SERVER");
        strcpy((char *)response_packet.data, "Join Successful.");
        response_packet.size = 17;

    } else {
        // Login failed, prepare lo_nak response
        response_packet.type = JN_NAK;
        strcpy((char *)response_packet.source, "SERVER");
        strcpy((char *)response_packet.data, "Join Unsuccessful.");
        response_packet.size = 19;
    }
    char response_buffer[BUFFERSIZE];
    memset(response_buffer, 0, BUFFERSIZE);
    packet_to_message(response_packet, response_buffer);
    response_buffer[strlen(response_buffer)] = '\0';
   
    if (send(sock, response_buffer, strlen(response_buffer), 0) < 0) {
        perror("send failed");
        return -1;
    }
    return join_status;
}

int attempt_leave(struct Client *self, struct Packet packet, int sock) {
    if (self->connected == 1 && self->in_session != 1) {
        printf("attempt_leave: client is either not online, or not in a session.\n");
        return -1;
    }
    else if (n_sessions == 0) {
        printf("attempt_leave: There are no session to leave.\n");
        return -1;
    }

    int sess_index = -1;
    for (int i = 0; i < n_sessions; i++) {
        if (strcmp(self->session_id, session_list[i].session_id) == 0) {
            sess_index = i;
            break;
        }
    }
    if (sess_index == -1) {
        printf("Session not found.\n");
        return -1;
    }
    // Find the client in the session's client list and remove them
    int client_index = -1;
    for (int i = 0; i < session_list[sess_index].n_clients_in_sess; i++) {
        if (strcmp(self->client_id, session_list[sess_index].clients_in_list[i]->client_id) == 0) {
            client_index = i;
            break;
        }
    }
    if (client_index == -1) {
        printf("Client not found in the session.\n");
        return -1;
    }

    if (session_list[sess_index].n_clients_in_sess-1 == 0){ // if that was the last client must delete session from session list
        if(sess_index == 0) {  // if that was the only session
            n_sessions = 0;
        }
        if(sess_index == n_sessions-1){ // if it was the last session in the session list simply pretend its not there
            n_sessions--;
        }
        else { // otherwise shift the sessions in order to delete it
            for (int i = sess_index; i < n_sessions; i++) {
                session_list[i] = session_list[i + 1];
                n_sessions--;
            }
        }
    }
    else if (client_index != session_list[sess_index].n_clients_in_sess - 1){ // if the client is not the last one in the client list
        for (int i = client_index; i < session_list[sess_index].n_clients_in_sess - 1; i++) {
            session_list[sess_index].clients_in_list[i] = session_list[sess_index].clients_in_list[i + 1];
        }
        session_list[sess_index].n_clients_in_sess--;
    }
    else {
        session_list[sess_index].n_clients_in_sess--;
    }
    self->in_session = 0;
    memset(self->session_id, 0, MAX_NAME);  // Assuming session_id is of length MAX_NAME
    
    return 0; 
}


int attempt_new(struct Client *self, struct Packet packet, int sock){
    bool status_ok = true;

    if (n_sessions == 0){
        struct Session newSession;
        strcpy(newSession.session_id, packet.data);
        newSession.clients_in_list[0] = self;
        newSession.n_clients_in_sess = 1;
        session_list[0] = newSession;
        n_sessions++;
        self->in_session = 1;
        strcpy(self->session_id, packet.data);
        printf("atempt_new: first session created!\n"
                            "User name: %s\n"
                            "User session_id: %s\n"
                            "session_list[0]: %s\n"
                            "session_list[0].clients_in_list[0]: %s\n", self->client_id, self->session_id, session_list[0].session_id, session_list[0].clients_in_list[0]->client_id);
    }
    else {
        for (int i = 0; i < n_sessions; i++) {
            if (strcmp(packet.data, session_list[i].session_id) == 0) {
                status_ok = false;
                break;  // No need to continue if the session is found
            }
        }
        if (status_ok && self->connected == 1 && self->in_session == 0) {
            struct Session newSession;
            strcpy(newSession.session_id, packet.data);
            newSession.clients_in_list[0] = self;
            newSession.n_clients_in_sess = 1;
            session_list[n_sessions] = newSession;
            n_sessions++;

            self->in_session = 1;
            strcpy(self->session_id, packet.data);

            printf("%s %s %d\n", self->client_id, self->session_id, self->in_session);
        }
        else { status_ok = false; }
    }

    struct Packet response_packet;
    memset(&response_packet, 0, sizeof(response_packet)); // Clear the response packet structure

    if (status_ok) {
        // Login successful, prepare lo_ack response
        response_packet.type = NS_ACK;
        strcpy((char *)response_packet.source, "SERVER");
        strcpy((char *)response_packet.data, "New Session Successful.");
        response_packet.size = strlen(response_packet.data);

    }
    char response_buffer[BUFFERSIZE];
    memset(response_buffer, 0, BUFFERSIZE);
    packet_to_message(response_packet, response_buffer);

    if (send(sock, response_buffer, strlen(response_buffer), 0) < 0) {
        perror("send failed");
        return -1;
    }
    else {    
        printf("attempt_new: created new session successfully.\n");
        return 0;
    }
}


int send_query_message(struct Client *self, int sock) {

    char message[BUFFERSIZE]; 
    strcpy(message, "Online Users:\n");

    // Iterate through clients to list online users
    for ( int i = 0; i < n_clients; i++) {
        if (client_list[i].connected) {
            strcat(message, client_list[i].client_id);
            strcat(message, "\n");
        }
    }
    strcat(message, "Available Sessions:\n");

    // Iterate through sessions to list available sessions
    for ( int i = 0; i < n_sessions; i++) {
        strcat(message, session_list[i].session_id);
        strcat(message, " - Clients: ");
        char numClients[10]; // To hold the number of clients in the session as string
        sprintf(numClients, "%d", session_list[i].n_clients_in_sess);
        strcat(message, numClients);
        strcat(message, "\n");
    }
    struct Packet response_packet;
    memset(&response_packet, 0, sizeof(response_packet)); // Clear the response packet structure

    response_packet.type = QU_ACK;
    strcpy((char *)response_packet.source, "SERVER");
    strcpy((char *)response_packet.data, message);
    response_packet.size = strlen(message);
    
    char response_buffer[BUFFERSIZE];
    memset(response_buffer, 0, BUFFERSIZE);
    packet_to_message(response_packet, response_buffer);
    response_buffer[strlen(response_buffer)] = '\0';

    if (send(sock, response_buffer, strlen(response_buffer), 0) < 0) {
        perror("send failed");
        return -1;
    }
    printf("%s", response_buffer);
    return 0;
}

void send_message(struct Client *self, struct Packet packet, int sock){
    int sess_index = 0;
    if (self->connected == 0 || self->in_session == 0) return;

    for (int i = 0; i < n_sessions; i++){
    }

        for (int j = 0; j < session_list[i].n_clients_in_sess; j++)
        {
            if (strcmp(session_list[i].clients_in_list[j]->client_id, self->client_id)==0)
            {
                sess_index = i;
                break;
            }
        }
    }
    for (int i = 0; i < session_list[sess_index].n_clients_in_sess; i++){

        if (strcmp(session_list[sess_index].clients_in_list[i]->client_id, self->client_id) != 0)
        {
            struct Packet response_packet;
            
            response_packet.type = MESSAGE;
            response_packet.size = strlen(packet.data);
            strcpy(response_packet.source, self->client_id);
            strcpy(response_packet.data, packet.data);

            char response_buffer[BUFFERSIZE];
            memset(response_buffer, 0, BUFFERSIZE);
            packet_to_message(response_packet, response_buffer);

            if (send(session_list[sess_index].clients_in_list[i]->socket_fd, response_buffer, strlen(response_buffer), 0) < 0) 
            {
                perror("send failed");
                return;
            }
        }
    }
}


void print_packet(struct Packet pack)
{
    printf("COMMAND: %d\nSIZE: %d\nNAME: %s\nDATA: %s\n", pack.type, pack.size, pack.source, pack.data);
}

void print_client(struct Client client)
{
    printf("ID: %s\nPASS: %s\nSESSION_ID: %s\nSOCKET: %d\nCONNECTED: %d\nIN_SESSION: %d\n", 
           client.client_id, client.password, client.session_id, client.socket_fd, 
           client.connected, client.in_session);
}

