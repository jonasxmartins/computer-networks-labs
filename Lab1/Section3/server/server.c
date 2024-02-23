#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include "structs/structs.h"
#include <stdbool.h>

/* This code was made with the help of Beej's Guide to Network Programming,
   especially section 5.1 - 5.8 and 6.3
   This resource can be found at https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch */

int packetsToData(struct packet *packetList, char *data);
bool acknowledge();

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

// should work until here

    printf("Server is running and waiting\n");

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

    unsigned int number_of_fragments = pack.total_frag;
    char filename[300]; 
    strcpy(filename, pack.filename);

    // send acknowledgement back
    char ack[BUFFERSIZE];
    int size_ack = makeAcknowledgement(pack.filename, pack.frag_no, ack);
    int bytes_sent = sendto(fd, ack, size_ack, 0, (struct sockaddr*)&sourceAddress, size);
    if (bytes_sent == -1)
    {
        perror("send acknowledge");
        exit(89);
    }
printf("This amount of fragments: %u\n", number_of_fragments);

    printf("bad bunny\n");
    // create list of packets
    while (pack.frag_no <= number_of_fragments)
    {
        printf("went here\n");
        int bytes_received = recvfrom(fd, (void*)buffer, BUFFERSIZE, 0, (struct sockaddr*)&sourceAddress, &size);
        if (bytes_received == -1)
        {
            perror("receive");
            exit(89);
        }
        pack = makeStruct(buffer, bytes_received);
        packetList[pack.frag_no - 1] = pack;
        printf("received %d packet\n", pack.frag_no);
        if (acknowledge())
        {
            int size_ack = makeAcknowledgement(pack.filename, pack.frag_no, ack);
            int bytes_sent = sendto(fd, ack, size_ack, 0, (struct sockaddr*)&sourceAddress, size);
            if (bytes_sent == -1)
            {
                perror("send acknowledge");
                exit(89);
            }
        }
        else
        {
            printf("packet number %u dropped\n", pack.frag_no);
        }
    }

    char *data;
    int data_size;

    if (number_of_fragments == 1)
    {
        data_size = packetList[0].size;
        data = (char*)malloc(data_size);
    }
    else
    {
        data_size = (number_of_fragments-1)*1000 +  packetList[number_of_fragments-1].size;
        data = (char*)malloc(data_size);
    }

    printf("The size of the file is %d bytes\n", data_size);

    packetsToData(packetList, data);
    FILE *out;

    printf("Opening file: %s\n", filename);


    out = fopen(filename, "wb");
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
        memcpy(data + i*1000, packetList[i].filedata, 1000);
    }

    memcpy(data + (frag_no -1)*1000, packetList[frag_no - 1].filedata, packetList[frag_no-1].size);

    return frag_no;
}

bool acknowledge()
{
    int random = rand()%100;
    if (random >= 50) 
    {
        printf("random is true %d\n", random);
        return true;
    }
    else
    {
        printf("random is false %d\n", random);
        return false;
    }
     
}