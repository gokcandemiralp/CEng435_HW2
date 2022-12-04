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
bool new_sent_message=false;
pthread_t listen_t;
pthread_t send_t;

int socket_desc;
struct sockaddr_in server_addr;
size_t packetsize = 100;
char *server_packet;
char *client_packet;
socklen_t server_struct_length = sizeof(server_addr);

int setSocket(void){
    server_packet = (char*) std::malloc(packetsize*sizeof(char));
    client_packet = (char*) std::malloc(packetsize*sizeof(char));
    
    memset(server_packet, '\0', sizeof(server_packet));
    memset(client_packet, '\0', sizeof(client_packet));
    
    // Create socket:
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
    
    return 0;
}

void* listen_routine(void* args){
    while(!terminate){
        if(recvfrom(socket_desc, server_packet, sizeof(server_packet), 0,
            (struct sockaddr*)&server_addr, &server_struct_length) < 0){
            printf("Error while receiving server's msg\n");
            return nullptr;
        }
        
        printf("Server's response: %s\n", server_packet);
        return nullptr; //delete
    }
    return nullptr;
}

void* send_routine(void* args){
    while(!terminate){
        if(new_sent_message){
            if(sendto(socket_desc, client_packet, strlen(client_packet), 0,
                (struct sockaddr*)&server_addr, server_struct_length) < 0){
                printf("Unable to send message\n");
                return nullptr;
            }
        }
    }
    return nullptr;
}

int loop(){
    int newLineCount = 0;
    while(1){
        printf("Enter message: ");
        getline(&client_packet, &packetsize ,stdin);
        if(!strcmp(client_packet,"\n")){
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
        new_sent_message = true;
    }
}

int main(void){
    setSocket();
    pthread_create(&listen_t, nullptr, &listen_routine, nullptr);
    pthread_create(&send_t, nullptr, &send_routine, nullptr);

    loop();

    pthread_join(listen_t, nullptr);
    pthread_join(send_t, nullptr);
}
