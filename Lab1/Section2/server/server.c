#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include "structs/structs.h"

/* This code was made with the help of Beej's Guide to Network Programming,
   especially section 5.1 - 5.8 and 6.3
   This resource can be found at https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch */

int packetsToData(struct packet *packetList, char *data);

int main(int argc, char *argv[]){
    
    // checks for the correct user input
    if (argc < 2)
    {
        fprintf(stderr, "Error: no arguments were passed\n");
        exit(4);
    }
    if (argc > 2)
    {
        fprintf(stderr, "Error: too many arguments were passed\n");
        exit(9);
    }

    // saves port number
    char* port = argv[1];

    int fd;
    int error;
    struct addrinfo info, *serverInfo, *final;
    char buffer[BUFFERSIZE];

    // initializes addrinfo struct that will be used for getaddrinfo
    memset(&info, 0, sizeof(info));
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_DGRAM;
    info.ai_flags = AI_PASSIVE;

    // gets a linked list of all the possible address that could work
    error = getaddrinfo(NULL, port, &info, &serverInfo);
    if (error != 0)
    {
        printf("error: %s\n", gai_strerror(error));
        exit(99);
    }

    // iterates over all the addresses obtained until finding one that works
    for (final = serverInfo; final != NULL; final = final->ai_next)
    {
        fd = socket(final->ai_family, final->ai_socktype, 0);
        if (fd == -1)
        {
            perror("server socket");
            continue;
        }

        if (bind(fd, final->ai_addr, final->ai_addrlen) == -1)
        {
            perror("bind");
            continue;
        }

        break;

    }

    if (final == NULL)
    {
        fprintf(stderr, "Error: couldn't find any address to bind");
        exit(8);
    }

    struct sockaddr_storage sourceAddress;
    socklen_t size = sizeof(sourceAddress);

    // wait to receive information from opened socket
    int bytes_received = recvfrom(fd, buffer, BUFFERSIZE, 0, (struct sockaddr*)&sourceAddress, &size);
    if (bytes_received == -1)
    {
        perror("receive");
        exit(89);
    }

    // start organizing received structs
    struct packet pack = makeStruct(buffer, bytes_received);
    struct packet *packetList = (struct packet*)malloc(pack.total_frag*sizeof(struct packet));
    packetList[0] = pack;

    int number_of_fragments = pack.total_frag;

    // send acknowledgement back
    char ack[BUFFERSIZE];
    int size_ack = makeAcknowledgement(pack.filename, pack.frag_no, ack);
    int bytes_sent = sendto(fd, ack, size_ack, 0, (struct sockaddr*)&sourceAddress, size);

    // create list of packets
    for (int i = 1; i < number_of_fragments; i++)
    {
        int bytes_received = recvfrom(fd, buffer, BUFFERSIZE, 0, (struct sockaddr*)&sourceAddress, &size);
        if (bytes_received == -1)
        {
            perror("receive");
            exit(89);
        }

        pack = makeStruct(buffer, bytes_received);

        packetList[i] = pack;

        int size_ack = makeAcknowledgement(pack.filename, pack.frag_no, ack);
        int bytes_sent = sendto(fd, ack, size_ack, 0, (struct sockaddr*)&sourceAddress, size);
        if (bytes_sent == -1)
        {
            perror("send acknowledge");
            exit(89);
        }
    }

    char *data;
    int data_size;

    if (number_of_fragments == 1)
    {
        data_size = packetList[0].size
        data = (char*)malloc(data_size);
    }
    else
    {
        data_size = (number_of_fragments - 1)*1000 +  packetList[number_of_fragments - 1].size;
        data = (char*)malloc(data_size);
    }

    packetsToData(packetList, data);

    FILE *out;

    out = fopen(packetList[0].filename, "w");
    if (out == NULL)
    {
        perror("file open");
        exit(77);
    }
    
    fwrite(data, data_size, 1, out);

    fclose(out);

    // frees up and closes all the necessary objects
    freeaddrinfo(serverInfo);
    free(packetList);
    free(data);

    

    return 0;
}

int packetsToData(struct packet *packetList, char *data)
{
    int frag_no = packetList[0].total_frag;

    for (int i = 0; i < frag_no - 1; i++)
    {
        for (int j = 0; j < 1000; j++)
        {
            data[j + 1000*i] = packetList[i].filedata[j];
        }
    }

    for (int j = 0; j < 1000; j++)
    {
        data[j + 1000*(frag_no - 1)] = packetList[frag_no - 1].filedata[j];
    }

    return frag_no;
}

