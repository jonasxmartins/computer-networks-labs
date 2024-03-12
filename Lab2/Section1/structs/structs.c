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