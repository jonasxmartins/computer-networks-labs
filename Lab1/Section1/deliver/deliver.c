#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>



int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s host_address port\n", argv[0]);
        return EXIT_FAILURE;
    }
    int socketfd = socket(AF_INET, SOCK_DGRAM, AF_INET);

    char input[64];
    fgets(input, sizeof(input), stdin);
    
    input[strcspn(input, "\n")] = '\0';
    char* command = strtok(input, " ");
    char* filename = strtok(NULL, " ");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET; // IPv4

    // Converts IP address from text to binary form
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    // Convert port number from string to int and then to network byte order
    int port = atoi(argv[2]);
    server_addr.sin_port = htons(port);

    if (command != NULL && strcmp(command, "ftp") == 0) {
        if (filename != NULL) {
            struct stat buffer;
            int exist = stat(filename, &buffer);
            if (exist == 0) {
                    char message[64] = "ftp\0";
                    if (sendto(socketfd, (const void *) &message, strlen(message)+1, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                        perror("sendto");
                        close(socketfd);
                        exit(EXIT_FAILURE);
                    }
                    char buf[64];

                    //line 56-57 not necessary, but good practice
                    struct sockaddr_storage src_addr;
                    socklen_t addr_len = sizeof(src_addr);
                    
                    if (recvfrom(socketfd, (void *)&buf, sizeof(buf), 0, (struct sockaddr*)&src_addr, &addr_len) < 0) {
                        perror("recvfrom");
                        close(socketfd);
                        exit(EXIT_FAILURE);
                    }
                    printf("%s", buf);
                    close(socketfd);
            } else {
                close(socketfd);
                exit(EXIT_FAILURE);
            }
        } else {
            close(socketfd);
            exit(EXIT_FAILURE);
        }
    } else {
        close(socketfd);
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}
