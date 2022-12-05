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

bool terminate=false;
pthread_t listen_t;
pthread_t send_t;

unsigned int packetCount = 0;
unsigned int ackCount = 0;
size_t packetsize = 16;
size_t contentsize = 12;
myPacket *server_packet;
myPacket *client_packet;

int socket_desc;
struct sockaddr_in server_addr;
socklen_t server_struct_length = sizeof(server_addr);
int port = 0;
char *terminal_ip;

int setSocket(void){
    server_packet = (myPacket*) std::malloc(sizeof(myPacket));
    client_packet = (myPacket*) std::malloc(sizeof(myPacket));
    
    memset(server_packet, '\0', sizeof(server_packet));
    memset(client_packet, '\0', sizeof(client_packet));
    
    // Create socket:
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(socket_desc < 0){
        perror("Error while creating socket\n");
        exit(EXIT_FAILURE);
    }
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr =  inet_addr(terminal_ip);
    
    return 0;
}

void* listen_routine(void* args){
    while(!terminate){
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

void* send_routine(void* args){
    int newLineCount = 0;
    char *tempBuffer = (char*) std::malloc(sizeof(char)*contentsize);
    while(1){
        getline(&tempBuffer, &packetsize ,stdin);
        strcpy(client_packet->content,tempBuffer);
        ++packetCount;
        client_packet->id = packetCount;
        if(!strcmp(client_packet->content,"\n")){
            ++newLineCount;
        }
        else{
            newLineCount = 0;
        }
        if(newLineCount == 3){ //termination condition
            terminate = true;
            close(socket_desc);
            return 0;
        }
        if(sendto(socket_desc, client_packet, packetsize, 0,
            (struct sockaddr*)&server_addr, server_struct_length) < 0){
            perror("Unable to send message\n");
            exit(EXIT_FAILURE);
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
