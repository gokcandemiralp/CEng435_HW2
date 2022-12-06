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
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <ctime>  

pthread_t listen_t;
pthread_t send_t;

unsigned int packetCount = 0;
unsigned int windowSize = 8;
unsigned int windowTail = 1;
unsigned int windowHead = windowTail + windowSize;

bool terminate = false;
bool wait_send = false;
bool wait_listen = true;

unsigned int lastValidID = 0;
size_t packetsize = 16;
size_t contentsize = 12;
myPacket *server_packet;
myPacket *client_packet;
myPacket *packetBuffer;

int socket_desc;
struct sockaddr_in server_addr, client_addr;
socklen_t client_struct_length = sizeof(client_addr);
int port = 2222;

int packetBufferManager(myPacket newPacket){
    for(int i = windowSize ; i>1 ; --i){ //going from top of the stack to bottom
        packetBuffer[i-1] = packetBuffer[i-2];
    }
    packetBuffer[0] = newPacket;
    return 0;
}

int begin(){
    server_packet = (myPacket*) std::malloc(sizeof(myPacket));
    client_packet = (myPacket*) std::malloc(sizeof(myPacket));
    packetBuffer = (myPacket*) std::malloc(sizeof(myPacket)*windowSize); //keep as many packets as window size

    return 0;
}

int setSocket(){

    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Generate the Socket
    
    if(socket_desc < 0){
        perror("Error while creating socket\n");
        exit(EXIT_FAILURE);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); //set the port
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //set the IP's
    
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){ // make the binding for setting port and IP
        perror("Couldn't bind to the port\n");
        exit(EXIT_FAILURE);
    }
    return 0;    
}

void* listen_routine(void* args){
    int newLineCount = 0;

    while(1){
        if(wait_send){continue;} //wait for send
        if (recvfrom(socket_desc,  client_packet, packetsize, 0,
            (struct sockaddr*)&client_addr, &client_struct_length) < 0){
            perror("Couldn't receive\n");
            exit(EXIT_FAILURE);
        }

        printf("%s",  client_packet->content);
        if(!strcmp(client_packet->content,"\n")){
            ++newLineCount;
        }
        else{
            newLineCount = 0;
        }
        if(newLineCount == 3){ //termination condition
            pthread_cancel(send_t);
            terminate = true;
            return nullptr;
        }
        wait_listen = false;
        wait_send = true;
    }
    return nullptr;
}

void* send_routine(void* args){
    while(!terminate){
        if(wait_listen){continue;} //wait for send
        server_packet = client_packet;
        
        if (sendto(socket_desc, server_packet, packetsize, 0,
            (struct sockaddr*)&client_addr, client_struct_length) < 0){
            perror("Can't send\n");
            exit(EXIT_FAILURE);
        }
        wait_listen = true;
        wait_send = false;
    }
    return nullptr;
}

int main(int argc, char** argv){
    //port = atoi(argv[1]);

    begin();
    setSocket();
    pthread_create(&listen_t, nullptr, &listen_routine, nullptr);
    pthread_create(&send_t, nullptr, &send_routine, nullptr);

    pthread_join(listen_t, nullptr);
    pthread_join(send_t, nullptr);
}
