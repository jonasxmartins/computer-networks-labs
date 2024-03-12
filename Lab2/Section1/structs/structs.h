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

// #define LOGIN_ACK 200
// #define LOGIN_NAK 400
// #define JOIN_ACK 202
// #define JOIN_NAK 402
// #define NEWSESSION_ACK 204
// #define NS_ACK 

#define CLIENT 69
#define SERVER 420

struct Packet {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];

};

struct Client {
    static char client_id[MAX_NAME];
    static char password[MAX_DATA];
    char session_id[MAX_DATA];
    int socket_fd;
    bool connected;
    bool in_session;
};

struct Session {
    static char session_id[MAX_NAME];
    struct Client *clients_in_list;
};

// Server side helper functions
int attempt_login(struct Client client_logging_in, struct Client *client_list);

int lo_ack(struct Client client);

int lo_nak(struct Client client);

int attempt_join(struct Client client, struct Session *session_list);

int attempt_leave(struct Client client, struct Session *session_list);

int attempt_new(struct Client client, struct Session *session_list);

int jn_ack(struct Client client);

int jn_nak(struct Client client);

struct Packet make_packet(int type, int size, unsigned char source[MAX_NAME], unsigned char data[MAX_DATA]);
 
char* make_query_message(struct Client *client_list, struct Session *session_list);

void packet_to_message(struct Packet packet, char *message);

struct Packet message_to_packet(char message[BUFFERSIZE]);











#endif
