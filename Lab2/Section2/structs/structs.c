#include "structs/structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>


struct Packet make_packet(int type, int size, unsigned char source[MAX_NAME], unsigned char data[MAX_DATA])
{
    struct Packet pack;
    pack.type = type;
    pack.size = strlen(data) + 1;
    strncpy(pack.source, source, MAX_NAME);
    strncpy(pack.data, data, MAX_DATA);

    return pack; 
}

void packet_to_message(struct Packet packet, char *message)
{
    snprintf(message, BUFFERSIZE, "%d:%d:%s:%s", packet.type, packet.size, packet.source, packet.data);
}

struct Packet message_to_packet(char message[BUFFERSIZE])
{
    printf("%s\n", message);
    struct Packet pack;
    pack.type = atoi(strtok(message, ":"));
    pack.size = atoi(strtok(NULL, ":"));
    strncpy(pack.source, strtok(NULL, ":"), MAX_NAME);
    strncpy(pack.data, strtok(NULL, ":"), MAX_DATA);

    return pack;
}


int attempt_login(int sock, struct Packet packet, struct Client *client_list, unsigned int n_clients) {
    int login_status = -1;

    if (packet.type != LOGIN) return -2;

    for (int i = 0; i < n_clients; i++) {
        if (strcmp(packet.source, client_list[i].client_id) == 0 &&
            strcmp(packet.data, client_list[i].password) == 0 &&
            (client_list[i].connected == 0)) {
            
            client_list[i].connected = 1;
            login_status = 0; // Login success
            break;
        }
    }
    // Prepare a response packet
    struct Packet response_packet;
    memset(&response_packet, 0, sizeof(response_packet)); // Clear the response packet structure

    if (login_status == 0) {
        // Login successful, prepare lo_ack response
        response_packet.type = LO_ACK;
        strcpy((char *)response_packet.source, "SERVER");
        strcpy((char *)response_packet.data, "Login Successful.");
        response_packet.size = 18;

    } else {
        // Login failed, prepare lo_nak response
        response_packet.type = LO_NAK;
        strcpy((char *)response_packet.source, "SERVER");
        strcpy((char *)response_packet.data, "Login Unsuccessful.");
        response_packet.size = 20;
    }

    // Convert response_packet to a string or binary format as per your protocol
    char response_buffer[BUFFERSIZE];
    memset(response_buffer, 0, BUFFERSIZE);
    // Assume packet_to_message does the conversion. Adjust as per your actual function signature and usage.
    packet_to_message(response_packet, response_buffer);
    response_buffer[strlen(response_buffer)] = '\0';
    // Send the response
    if (send(sock, response_buffer, strlen(response_buffer), 0) < 0) {
        perror("send failed");
        return -1;
    }
    return login_status;
}

int attempt_join(struct Client *self, struct Packet packet, struct Session *session_list, unsigned int n_sessions, int sock){
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
        strcpy(self->session_id, packet.data);
        self->in_session = 1;
        int n_cli = session_list[sess_index].n_clients_in_sess;
        session_list[sess_index].clients_in_list[n_cli] = *self; 
        session_list[sess_index].n_clients_in_sess += 1;
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

int attempt_leave(struct Client *self, struct Packet packet, struct Session *session_list, unsigned int n_sessions, int sock) {
    if (self->connected != 1 || self->in_session != 1) {
        printf("Client is not in a session.\n");
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
        if (strcmp(self->client_id, session_list[sess_index].clients_in_list[i].client_id) == 0) {
            client_index = i;
            break;
        }
    }

    if (client_index == -1) {
        printf("Client not found in the session.\n");
        return -1;
    }

    // Shift remaining clients in the session's client list
    for (int i = client_index; i < session_list[sess_index].n_clients_in_sess - 1; i++) {
        session_list[sess_index].clients_in_list[i] = session_list[sess_index].clients_in_list[i + 1];
    }
    session_list[sess_index].n_clients_in_sess--;

    self->in_session = 0;
    memset(self->session_id, 0, MAX_NAME);  // Assuming session_id is of length MAX_NAME

    return 0; 
}


int attempt_new(struct Client *self, struct Packet packet, struct Session *session_list, unsigned int n_sessions, int sock){
    // int new_status = -1;
    // bool session_exists = false;
    // for (int i = 0; i < n_sessions; i++) {
    //     if (strcmp(packet.data, session_list[i].session_id) == 0) {
    //         session_exists = true;
    //         break;  // No need to continue if the session is found
    //     }
    // }

    // if (!session_exists && self->connected == 1 && self->in_session == 0) {
    //     new_status = 0;
    //     strcpy(self->session_id, packet.data);
    //     self->in_session = 1;
    //     struct Session new_session;
    //     new_session.session_id = self->session_id;
    //     struct Client clients[MAX_DATA];
    //     clients[0] = *self;
    //     new_session.clients_in_list = clients;_

    //     strcpy(new_session.session_id, packet.data);
    //     new_session.n_clients_in_sess = 1;
    //     new_session.clients_in_list[0] = *self;
    //     session_list[n_sessions] = new_session; // Assuming there's enough space
    //     printf("%s %s %d\n", self->client_id, self->session_id, self->in_session);
    //     n_sessions++;  // Increment the total session count
    // }

    // struct Packet response_packet;
    // memset(&response_packet, 0, sizeof(response_packet)); // Clear the response packet structure

    // if (new_status == 0) {
    //     // Login successful, prepare lo_ack response
    //     response_packet.type = NS_ACK;
    //     strcpy((char *)response_packet.source, "SERVER");
    //     strcpy((char *)response_packet.data, "New Session Successful.");
    //     response_packet.size = 24;

    // }
    // char response_buffer[BUFFERSIZE];
    // memset(response_buffer, 0, BUFFERSIZE);
    // packet_to_message(response_packet, response_buffer);
    // response_buffer[strlen(response_buffer)] = '\0';
    // if (send(sock, response_buffer, strlen(response_buffer), 0) < 0) {
    //     perror("send failed");
    //     return -1;
    // }
    // printf("%d\n", new_status);
    return 0;
}

int send_query_message(struct Client *self, 
                       struct Client *client_list, 
                       struct Session *session_list, 
                       unsigned int n_sessions, 
                       unsigned int n_clients
                       , int sock) {

    char message[BUFFERSIZE] = {0}; // Assuming BUFFERSIZE is defined and sufficient
    strcat(message, "Online Users:\n");

    // Iterate through clients to list online users
    for (unsigned int i = 0; i < n_clients; i++) {
        if (client_list[i].connected) {
            strcat(message, client_list[i].client_id);
            strcat(message, "\n");
        }
    }
    strcat(message, "Available Sessions:\n");

    // Iterate through sessions to list available sessions
    for (unsigned int i = 0; i < n_sessions; i++) {
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

void send_message(struct Client *self, struct Packet packet, struct Session *session_list, unsigned int n_sessions, int sock){
    int sess_index = 0;
    if (self->connected == 0 || self->in_session == 0) return;

    for (int i = 0; i < n_sessions; i++){
        if (strcmp(packet.data, session_list[i].session_id) == 0) sess_index = i;
    }

    for (int i = 0; i < session_list[sess_index].n_clients_in_sess; i++){

        if (strcmp(session_list[sess_index].clients_in_list[i].client_id, self->client_id) != 0){
            
            struct Packet response_packet;
            memset(&response_packet, 0, sizeof(response_packet)); // Clear the response packet structure
            
            response_packet.type = MESSAGE;
            response_packet.size = strlen(packet.data);
            strcpy((char *)response_packet.source, self->client_id);
            strcpy((char *)response_packet.data, packet.data);
            response_packet.size = strlen(packet.data);
            
            char response_buffer[BUFFERSIZE];
            memset(response_buffer, 0, BUFFERSIZE);
            packet_to_message(response_packet, response_buffer);

            if (send(session_list[sess_index].clients_in_list[i].socket_fd, response_buffer, strlen(response_buffer), 0) < 0) {
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

