#ifndef structs_h
#define structs_h

#define BUFFERSIZE 2048
#define MAX_NAME 64
#define MAX_DATA 1024
#define LOGIN 1
#define LO_ACK 2
#define LO_NAK 3
#define EXIT 4
#define JOIN 5
#define JN_ACK 6
#define JN_NAK 7
#define LEAVE_SESS 8
#define NEW_SESS 9
#define NS_ACK 10
#define MESSAGE 11
#define QUERY 12
#define QU_ACK 13

#define CLIENT 69
#define SERVER 420

struct Packet {
    int type;
    int size;
    char source[MAX_NAME];
    char data[MAX_DATA];
};

struct Client {
    char client_id[MAX_NAME];
    char password[MAX_DATA];
    char session_id[MAX_DATA];
    int socket_fd;
    int connected;
    int in_session;
};

struct Session {
    char session_id[MAX_DATA];
    struct Client * clients_in_list[16];
    int n_clients_in_sess;
};

// Server side helper functions
struct Packet make_packet(int type, int size, char source[MAX_NAME], char data[MAX_DATA]);
struct Packet message_to_packet(char message[BUFFERSIZE]);
void packet_to_message(struct Packet packet, char *message);

#endif
