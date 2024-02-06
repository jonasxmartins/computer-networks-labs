#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>


#define BUFFERSIZE 2048

/* This code was made with the help of Beej's Guide to Network Programming,
   especially section 5.1 - 5.8 and 6.3
   This resource can be found at https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch */

struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
}

struct packet makeStruct(char buffer[BUFFERSIZE], int size);

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

    struct packet pack = 


    // sends answer back
    int bytes_sent = sendto(fd, returnMsg, strlen(returnMsg)+1, 0, (struct sockaddr*)&sourceAddress, size);

    // frees up and closes all the necessary objects
    freeaddrinfo(serverInfo);

    

    return 0;
}


struct packet makeStruct(char buffer[BUFFERSIZE], int size)
{
    buffer[size] = '\0';

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
    pack.filedata = filedata;

    return pack;
}