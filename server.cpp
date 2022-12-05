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

bool terminate = false;
pthread_t listen_t;
pthread_t send_t;

unsigned int packetCount = 0;
unsigned int ackCount = 0;
size_t packetsize = 16;
size_t contentsize = 12;
myPacket *server_packet;
myPacket *client_packet;

int socket_desc;
struct sockaddr_in server_addr, client_addr;
socklen_t client_struct_length = sizeof(client_addr);
int port = 0;

int setSocket(){
    server_packet = (myPacket*) std::malloc(sizeof(myPacket));
    client_packet = (myPacket*) std::malloc(sizeof(myPacket));

    memset(server_packet, '\0', sizeof(server_packet));
    memset( client_packet, '\0', sizeof( client_packet));

    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(socket_desc < 0){
        perror("Error while creating socket\n");
        exit(EXIT_FAILURE);
    }
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Couldn't bind to the port\n");
        exit(EXIT_FAILURE);
    }
    return 0;    
}

void* listen_routine(void* args){
    int newLineCount = 0;

    while(!terminate){
        if (recvfrom(socket_desc,  client_packet, packetsize, 0,
            (struct sockaddr*)&client_addr, &client_struct_length) < 0){
            perror("Couldn't receive\n");
            exit(EXIT_FAILURE);
        }
        if(packetCount = client_packet->id){
            continue;
        }
        else{
            packetCount = client_packet->id;
        }

        if(!strcmp(client_packet->content,"\n")){
            ++newLineCount;
        }
        else{
            newLineCount = 0;
            ++packetCount;
        }
        if(newLineCount == 3){ //termination condition
            terminate = true;
            close(socket_desc);
            return 0;
        }
    }
    return nullptr;
}

void* send_routine(void* args){
    while(!terminate){
        if(packetCount>ackCount){
            printf("Msg from client: %s\n",  client_packet->content);
            server_packet = client_packet;
            
            if (sendto(socket_desc, server_packet, packetsize, 0,
                (struct sockaddr*)&client_addr, client_struct_length) < 0){
                perror("Can't send\n");
                exit(EXIT_FAILURE);
            }
            ++ackCount;
        }
    }
    return nullptr;
}

int main(int argc, char** argv){
    port = atoi(argv[1]);

    setSocket();
    pthread_create(&listen_t, nullptr, &listen_routine, nullptr);
    pthread_create(&send_t, nullptr, &send_routine, nullptr);

    pthread_join(listen_t, nullptr);
    pthread_join(send_t, nullptr);
}
