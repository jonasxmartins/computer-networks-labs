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

void makeMessage(struct packet *pack,  char message[sizeof(struct packet)])
{
    int check = snprintf(message, sizeof(struct packet), "%u%u%u%s%s", 
                pack->total_frag, pack->frag_no, pack->size, 
                pack->filename, pack->filedata);
    if (check <= 0) 
    {
        perror("make message");
        exit(888);
    }
}