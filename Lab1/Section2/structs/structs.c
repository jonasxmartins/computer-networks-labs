#include "structs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct packet makeStruct(char buffer[BUFFERSIZE], int num_bytes)
{
    buffer[num_bytes] = '\0';

    char* totalFrag = strtok(buffer, ":");
    char* fragNo = strtok(NULL, ":");
    char* size = strtok(NULL, ":");
    char* filename = strtok(NULL, ":");
    char* filedata = strtok(NULL, ":");

    struct packet pack;

    pack.total_frag = atoi(totalFrag);
    pack.frag_no = atoi(fragNo);
    pack.size = atoi(size);
    pack.filename = filename;
    strncpy(pack.filedata, filedata, 1000);

    return pack;
}

int makeMessage(struct packet *pack,  char message[BUFFERSIZE])
{
    int check = snprintf(message, BUFFERSIZE, "%u:%u:%u:%s:%s", 
                pack->total_frag, pack->frag_no, pack->size, 
                pack->filename, pack->filedata);
    if (check <= 0) 
    {
        perror("make message");
        exit(888);
    }
    return check;
}

struct ack acknowledgeToPacket(char buffer[BUFFERSIZE], int num_bytes)
{
    buffer[num_bytes] = '\0';

    char* fragNo = strtok(buffer, ":");
    char* filename = strtok(NULL, ":");

    struct ack pack;

    pack.filename = filename;
    pack.frag_no = atoi(fragNo);

    return pack;

}

int makeAcknowledgement(char* filename, unsigned int frag_no, char message[BUFFERSIZE])
{
        int check = snprintf(message, BUFFERSIZE, "%s:%u", filename, frag_no);

        if (check <= 0) 
        {
            perror("make ack");
            exit(888);
        }
        return check;
}