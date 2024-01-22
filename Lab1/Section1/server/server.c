#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>

int main(){
    printf("Hello worlddddd\n");

    int fd = socket(AF_INET, SOCK_DGRAM, AF_INET);

    return 0;
}