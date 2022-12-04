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

int socket_desc;
struct sockaddr_in server_addr, client_addr;
size_t buffersize = 100;
char *server_packet;
char *client_packet;
socklen_t client_struct_length = sizeof(client_addr);

int setSocket(){
    server_packet = (char*) std::malloc(buffersize*sizeof(char));
     client_packet = (char*) std::malloc(buffersize*sizeof(char));

    memset(server_packet, '\0', sizeof(server_packet));
    memset( client_packet, '\0', sizeof( client_packet));

    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");
    return 0;    
}

void* listen_routine(void* args){
    int newLineCount = 0;
    while(!terminate){
        if (recvfrom(socket_desc,  client_packet, sizeof( client_packet), 0,
            (struct sockaddr*)&client_addr, &client_struct_length) < 0){
            printf("Couldn't receive\n");
            return nullptr;
        }

        if(!strcmp(client_packet,"\n")){
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
        
        printf("Received message from IP: %s and port: %i\n",
        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
    }
    return nullptr;
}

void* send_routine(void* args){
    while(!terminate){
        if(packetCount>ackCount){
            printf("Msg from client: %s\n",  client_packet);
            strcpy(server_packet,  client_packet);
            
            if (sendto(socket_desc, server_packet, strlen(server_packet), 0,
                (struct sockaddr*)&client_addr, client_struct_length) < 0){
                printf("Can't send\n");
                return nullptr;
            }
            ++ackCount;
            printf("%d\n",ackCount);
        }
    }
    return nullptr;
}

int main(void){
    setSocket();
    pthread_create(&listen_t, nullptr, &listen_routine, nullptr);
    pthread_create(&send_t, nullptr, &send_routine, nullptr);

    pthread_join(listen_t, nullptr);
    pthread_join(send_t, nullptr);
}
