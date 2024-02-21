#ifndef structs_h
#define structs_h

#define BUFFERSIZE 2048

struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

struct ack{
    char* filename;
    unsigned int frag_no;
};

struct packet makeStruct(char buffer[BUFFERSIZE], int num_bytes);

int makeMessage(struct packet *pack, char message[BUFFERSIZE]);


struct ack acknowledgeToPacket(char buffer[BUFFERSIZE], int num_bytes);

int makeAcknowledgement(char* filename, unsigned int frag_no, char message[BUFFERSIZE]);



#endif
