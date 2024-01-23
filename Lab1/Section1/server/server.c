#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>

int main(){

    int fd = socket(AF_INET, SOCK_DGRAM, AF_INET);

    const struct sockaddr *addr;

    if (bind(fd, addr, sizeof(addr)) == -1)
    {
        perror("bind ain't working");
        exit(77);
    }

    return 0;
}