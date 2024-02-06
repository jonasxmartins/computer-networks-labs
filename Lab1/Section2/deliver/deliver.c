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
#include <sys/time.h>
#include <time.h>
#include "structs/structs.h"
#include <math.h>

#define SECONDS_IN_MICRO 1000000
// RTT is 617 microseconds
struct timeval start, end;

struct packet dataToPacket(const unsigned int totalFrag, 
                           const unsigned int f_no, 
                           size_t size,
                           char *filename,
                           char *buffer
                           );

size_t fsize(FILE *fp);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s host_address port\n", argv[0]);
        return EXIT_FAILURE;
    }
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);

    printf("Input the name of a file to tranfer in the following form:\n\n      ftp <filename>\n\n");

    char input[64];
    fgets(input, sizeof(input), stdin);
    
    input[strcspn(input, "\n")] = '\0';
    char* command = strtok(input, " "); // stores command based on user input
    char* filename = strtok(NULL, " "); // stores filename based on user input


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
    
    if (command != NULL) 
    {
        if (filename != NULL) 
        {
            struct stat buffer;
            int exist = stat(filename, &buffer);
            if (exist == 0) 
            {   
                // opens file and calls function to know size of file and how many fragments
                FILE *fptr;
                fptr = fopen(filename, "rb");
                size_t size = fsize(fptr);
                const unsigned int totalFrag = (const unsigned int) ceil((double)size / 1000.00);
                /*  this is the loop where the fragments are sent out:
                        right now each fragment is created on the spot as the file
                        data is being read, meaning it is very memory efficient but
                        not optimal for speed, but I can implement non-blocking recv
                        or change it to before sendto so the struct is made before
                        waiting for recv, a lot of options available */
                // printf("File size = %d\n%d fragments\n", size, totalFrag);
                for (int f_no = 1; f_no <= totalFrag; f_no++){
                    
                    // gets the size of the data in the fragment
                    unsigned int f_size;
                    if (totalFrag == 1)
                        f_size = size;
                    else if (f_no == totalFrag && (size % 1000) != 0)
                        f_size = size % ((totalFrag - 1) * 1000);
                    else f_size = 1000;

                    //printf("Fragment Size = %d\n", f_size);

                    char buffer[f_size];
                    int o = fread(buffer, sizeof(char), f_size, fptr);
                    if (!o){
                        perror("fread");
                        exit(88);
                    }
                    struct packet f_packet = dataToPacket(totalFrag, f_no, f_size, filename, buffer);
                    char message[BUFFERSIZE];
                    int check = (&f_packet, message);

                    if (f_no == 1) gettimeofday(&start, NULL);

                    if (sendto(socketfd, (const void *) &message, BUFFERSIZE, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                        perror("sendto");
                        close(socketfd);
                        exit(EXIT_FAILURE);
                    }


                    // receiving the acknowledgement 
                    char buf[BUFFERSIZE];
                    struct sockaddr_storage src_addr;
                    socklen_t addr_len = sizeof(src_addr);
                    
                    int bytes_recv = recvfrom(socketfd, (void *)&buf, sizeof(buf), 0, (struct sockaddr*)&src_addr, &addr_len);

                    if ( bytes_recv < 0) {
                        perror("recvfrom");
                        close(socketfd);
                        exit(EXIT_FAILURE);
                    }
                    if (f_no == totalFrag) gettimeofday(&end, NULL);

                    buf[bytes_recv] = '\0';

                    puts(buf); // prints acknowledgement


                }
                close(socketfd);
            } 
            else 
            {
                fprintf(stderr, "error: file does not exist\n");
                close(socketfd);
                exit(EXIT_FAILURE);
            }
        }
        else 
        {
            close(socketfd);
            exit(EXIT_FAILURE);
        }
    } 
    else 
    {
        close(socketfd);
        exit(EXIT_FAILURE);
    }

    double seconds = difftime(end.tv_sec, start.tv_sec);

    
    long int microseconds = (long int)(seconds*SECONDS_IN_MICRO + end.tv_usec - start.tv_usec);

    //printf("%li microseconds passed.\n", microseconds); 


    return EXIT_SUCCESS;
}
        

struct packet dataToPacket(const unsigned int totalFrag, 
                           const unsigned int f_no, 
                           size_t size,
                           char *filename,
                           char *buffer
                           ){
    struct packet packet;

    packet.total_frag = totalFrag;
    packet.frag_no = f_no;
    packet.size = (unsigned int) size;
    packet.filename = filename;

    memcpy(packet.filedata, buffer, size);
    return packet;
}

size_t fsize(FILE *fp){ // gets size of a file
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were

    return (size_t) sz;
}