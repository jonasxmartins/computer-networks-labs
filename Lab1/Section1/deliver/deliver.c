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
    
    printf("Hello");

    return EXIT_SUCCESS;
}
