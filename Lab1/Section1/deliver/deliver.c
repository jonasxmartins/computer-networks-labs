#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s hostaddress port\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    // handle error
    getaddrinfo(*argv[1], *argv[2], &hints, &res);
    // res now points to a linked list of 1 or more struct addrinfos

    // ... create socket and use res for the address in sendto

    freeaddrinfo(res); // free the linked list when done

    int getaddrinfo()


    
    int fd = socket(AF_INET, SOCK_DGRAM, AF_INET);

    

    return EXIT_SUCCESS;
}
