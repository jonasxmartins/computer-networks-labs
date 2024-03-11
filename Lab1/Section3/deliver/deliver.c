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
#include <stdbool.h>

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

void recalc_timeout(float * timeout_val, float * current_rtt, float * all_rtts, int * n_rtts); // this function calculates E[RTT] and STD[RTT]

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
                printf("\n*****INITIATING TRANSMISSION*****\n\n");
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

                // value of timeout and current RTT value, represented by difference
                float timeout_val, difference;
                timeout_val = 5.0; // intial value is 5 seconds

                clock_t before;
                float all_rtts[10000]; // all the RTTs so far, used for calculating time out
                int n_rtts = 0; // number of RTTs so far, useful for indexing in time out calculation

                int acknowledged = 0;
                bool retransmitting = false;
                char message[BUFFERSIZE];
                while(acknowledged < totalFrag)
                {
                    unsigned int f_size;
                    if (totalFrag == 1)
                        f_size = size;
                    else if (acknowledged + 1 == totalFrag && (size % 1000) != 0)
                        f_size = size % ((totalFrag - 1) * 1000);
                    else f_size = 1000;
                        
                    if (!retransmitting)
                    {    // gets the size of the data in the fragment
                        char buffer[f_size];
                        int o = fread(buffer, sizeof(char), f_size, fptr);
                        if (!o){
                            perror("fread");
                            exit(88);
                        }
                        
                        struct packet f_packet = dataToPacket(totalFrag, acknowledged + 1, f_size, filename, buffer);
                        
                        int check = makeMessage(&f_packet, message);
                    }
                    
                    float extra = 0;
                    
                    if (sendto(socketfd, (const void *) &message, BUFFERSIZE, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                        perror("sendto");
                        close(socketfd);
                        exit(EXIT_FAILURE);
                    }
                    
                    before = clock();

                    // receiving the acknowledgement 
                    char buf[BUFFERSIZE];

                    struct sockaddr_storage src_addr;
                    socklen_t addr_len = sizeof(src_addr);

                    int bytes_recv = -1;
                    
                    while (bytes_recv == -1) 
                    {
                        bytes_recv = recvfrom(socketfd, (void *)&buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&src_addr, &addr_len);
                        difference = (clock() - before) / (float)CLOCKS_PER_SEC;
                        if (difference > timeout_val){
                            printf("\nRetransmitting packet number: %d\n\n", acknowledged+1);
                            retransmitting = true;
                            break;
                        }
                    }
                    printf("timeout is %f\n", timeout_val);
                    if (bytes_recv != -1)
                    {
                        all_rtts[n_rtts] = difference + extra;
                        extra = 0;
                        n_rtts++;
                        recalc_timeout(&timeout_val, &difference, all_rtts, &n_rtts);
                        struct ack acknowledgement = acknowledgeToPacket(buf, bytes_recv);
                        printf("Received acknowledgement for packet number %d\n", acknowledgement.frag_no);
                        acknowledged++;
                        retransmitting = false;
                    }
                    else
                    {
                        extra += timeout_val;
                    }


                    buf[bytes_recv] = '\0';




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

void recalc_timeout(float * timeout_val, float * current_rtt, float * all_rtts, int * n_rtts){

    // upon the first time out, the first RTT becomes the timeout as there is no standard deviation
    if (* n_rtts == 0) return;
    else if (* n_rtts == 1){
        *timeout_val = *current_rtt;
        return;
    }

    // calculates the sum of all the rtts thus far, for averaging
    float total_rtts;
    for (int i = 0; i < * n_rtts; i++){
        total_rtts += all_rtts[i];
    } 
    // this is the new average
    float new_avg = ( total_rtts + (*current_rtt) ) / ((float)* n_rtts);

    // calculates the STD
    float SD = 0;
    for (int i = 0; i < * n_rtts; i++){
        SD += pow(all_rtts[i] - new_avg, 2);
    }
    SD = sqrt(SD / (float) * n_rtts);

    * timeout_val = new_avg + 4 * SD;

}

