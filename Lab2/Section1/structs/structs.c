#include "structs.h"
#include <string.h>

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
    snprintf(message, BUFFERSIZE, "%s:%d:%s:%s", packet.data, packet.size, packet.source, packet.data);
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