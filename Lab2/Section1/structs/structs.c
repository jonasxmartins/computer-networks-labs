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