#include "common.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdio>
#include <pthread.h>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>

pthread_t listen_t;
pthread_t send_t;

unsigned int packetCount = 0;
unsigned int ackCount = 0;
size_t packetsize = 16;
size_t contentsize = 12;
size_t buffersize = 1024;
myPacket *server_packet;
myPacket *client_packet;
char *buffer;

int socket_desc;
struct sockaddr_in server_addr;
socklen_t server_struct_length = sizeof(server_addr);
int port = 0;
char *terminal_ip;

int setSocket(void){
    server_packet = (myPacket*) std::malloc(sizeof(myPacket));
    client_packet = (myPacket*) std::malloc(sizeof(myPacket));
    buffer = (char*) std::malloc(sizeof(char)*1024);
    
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Generate the Socket
    
    if(socket_desc < 0){
        perror("Error while creating socket\n");
        exit(EXIT_FAILURE);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); //set the port
    server_addr.sin_addr.s_addr =  inet_addr(terminal_ip); //set the IP
    
    return 0;
}

void* listen_routine(void* args){
    while(1){
        if(recvfrom(socket_desc, server_packet, sizeof(server_packet), 0,
            (struct sockaddr*)&server_addr, &server_struct_length) < 0){
            perror("Error while receiving server's msg\n");
            exit(EXIT_FAILURE);
            return nullptr;
        }
        
        printf("Ack Number: %d\n", server_packet->id);
    }
    return nullptr;
}

int clipper(char (&c_content)[12], char *buffer, int start, int end){
    for(int i = start ; i < end ; ++i){
        c_content[i%12] = buffer[i];
    }
    return 0;
}

void* send_routine(void* args){
    int newLineCount = 0;
    int string_len = 0;
    int packetDivs = 1;

    while(1){
        getline(&buffer, &buffersize ,stdin);
        packetDivs = (strlen(buffer)/12+1);

        for(int i = 0 ; i<packetDivs; ++i){
            ++packetCount;
            client_packet->id = packetCount;
            clipper(client_packet->content,buffer,i,(i+1)*12);
            if(sendto(socket_desc, client_packet, packetsize, 0,
                (struct sockaddr*)&server_addr, server_struct_length) < 0){
                perror("Unable to send message\n");
                exit(EXIT_FAILURE);
            }
        }
        if(!strcmp(buffer,"\n")){
            ++newLineCount;
        }
        else{
            newLineCount = 0;
        }
        if(newLineCount == 3){ //termination condition
            pthread_cancel(listen_t);
            close(socket_desc);
            return 0;
        }
    }
}

int main(int argc, char** argv){
    port = atoi(argv[1]);
    terminal_ip = argv[2];

    setSocket();
    pthread_create(&listen_t, nullptr, &listen_routine, nullptr);
    pthread_create(&send_t, nullptr, &send_routine, nullptr);

    pthread_join(listen_t, nullptr);
    pthread_join(send_t, nullptr);
}
