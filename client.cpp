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

pthread_t listen_t,send_t,backn_t;

unsigned int packetCount = 0;
unsigned int windowSize = 8;
unsigned int windowTail = 1;
unsigned int windowHead = windowTail + windowSize - 1;

std::chrono::_V2::system_clock::time_point tailTimeout;
float timeoutDuration = 0.211;

size_t packetsize = 16;
size_t contentsize = 12;
size_t buffersize = 1032; // it is 1032 and not 1024 becuse it is divisible by 12
myPacket *server_packet;
myPacket *client_packet;
myPacket *packetStack;
int stackSize = 0;
char *buffer;
std::vector<unsigned int> ackVector;

int socket_desc;
struct sockaddr_in server_addr;
socklen_t server_struct_length = sizeof(server_addr);
int port = 2222;

int packetStackManager(myPacket newPacket){
    for(int i = windowSize ; i>1 ; --i){ //going from top of the stack to bottom
        packetStack[i-1] = packetStack[i-2];
    }
    packetStack[0] = newPacket;
    if(stackSize<8){++stackSize;}
    return 0;
}

int printStack(){
    for(int i = stackSize - 1 ; i>=0 ; --i){ //going from top of the stack to bottom
        printf("packetStack[%d]: %d \n",i,packetStack[i].id);
    }
    return 0;
}

int begin(){
    server_packet = (myPacket*) std::malloc(sizeof(myPacket));
    client_packet = (myPacket*) std::malloc(sizeof(myPacket));
    packetStack = (myPacket*) std::malloc(sizeof(myPacket)*windowSize); //keep as many packets as window size
    buffer = (char*) std::malloc(sizeof(char)*1032); // it is 1032 and not 1024 becuse it is divisible by 12

    return 0;
}

int setSocket(void){
  
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Generate the Socket
    
    if(socket_desc < 0){
        perror("Error while creating socket\n");
        exit(EXIT_FAILURE);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); //set the port
    server_addr.sin_addr.s_addr =  inet_addr("172.24.0.10"); //set the IP
    
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
        if(server_packet->id >= windowTail){ //if it is a significant package
            if(!std::count(ackVector.begin(), ackVector.end(), server_packet->id)){ackVector.push_back(server_packet->id);} //if first time save ack
            if(windowTail == server_packet->id || std::count(ackVector.begin(), ackVector.end(), windowTail)){ //check if tail ack is received
                ackVector.erase(std::remove(ackVector.begin(), ackVector.end(), windowTail), ackVector.end()); //delete old tail
                windowTail = windowTail + 1; //the last received Ack is pushed out of the window
                windowHead = windowTail + windowSize - 1; //the next packet is added to the window and it is now sendable
                tailTimeout = std::chrono::system_clock::now(); //reset the timeout value to now
            }
        }
        printf("[tail: %d| head: %d] Ack Number: %d \n",windowTail,windowHead, server_packet->id);
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
            while(packetCount >= windowHead){continue;} //only sending packets up to window head
            ++packetCount;
            client_packet->id = packetCount;
            clipper(client_packet->content,buffer,i,(i+1)*12);
            packetStackManager(*client_packet); //add the to be sent package to the window package stack
            if(sendto(socket_desc, client_packet, packetsize, 0,
                (struct sockaddr*)&server_addr, server_struct_length) < 0){
                perror("Unable to send message\n");
                exit(EXIT_FAILURE);
            }
            tailTimeout = std::chrono::system_clock::now(); //set the timer if a package is sent
        }
        if(!strcmp(buffer,"\n")){
            ++newLineCount;
        }
        else{
            newLineCount = 0;
        }
        if(newLineCount == 3){ //termination condition
            pthread_cancel(listen_t);
            pthread_cancel(backn_t);
            close(socket_desc);
            return 0;
        }
    }
}

void* backn_routine(void* args){
    while(1){
        if(packetCount != windowTail - 1 ){ //if there are still packets pending
            std::chrono::duration<double> elapsedTime = std::chrono::system_clock::now() - tailTimeout;
            if(elapsedTime.count() > timeoutDuration){
                printf("Packet: %d timed out\n",windowTail);
                for(int i = stackSize-1 ; i >= 0 ; --i){
                    myPacket sendPacket = packetStack[i];
                    if(sendto(socket_desc, &sendPacket, packetsize, 0,
                            (struct sockaddr*)&server_addr, server_struct_length) < 0){
                            perror("Unable to send message\n");
                            exit(EXIT_FAILURE);
                    }
                }
                tailTimeout = std::chrono::system_clock::now();
            }
        }
        else{
        tailTimeout = std::chrono::system_clock::now();
        }
    }
}

int main(int argc, char** argv){
    //terminal_ip = argv[1];
    //port = atoi(argv[2]);

    begin();
    setSocket();
    pthread_create(&listen_t, nullptr, &listen_routine, nullptr);
    pthread_create(&send_t, nullptr, &send_routine, nullptr);
    pthread_create(&backn_t, nullptr, &backn_routine, nullptr);

    pthread_join(backn_t, nullptr);
    pthread_join(listen_t, nullptr);
    pthread_join(send_t, nullptr);
}
