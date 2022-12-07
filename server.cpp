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

//----------------Some_Socket_Declerations--------------------

int socket_desc;
struct sockaddr_in server_addr, client_addr;

//----------------Some_Constants--------------------

unsigned int windowSize = 8;
size_t packetsize = 16;
size_t contentsize = 11;
int port = 2222;
socklen_t client_struct_length = sizeof(client_addr);
float timeoutDuration = 0.211;
size_t inputBufferSize = 1034; // it is 1034 and not 1024 becuse it is divisible by 11

//----------------Global_Variables--------------------

pthread_t listen_t,send_packet_t,send_ack_t,backn_t;

unsigned int S_lastValidID = 0; //send progress counter
unsigned int R_lastValidID = 1; //receive progress counter
unsigned int windowTail = 1;
unsigned int windowHead = windowTail + windowSize - 1;

bool terminate = false;
bool wait_ack_send = false;
bool wait_ack_listen = true;

std::chrono::_V2::system_clock::time_point tailTimeout;

int newLineCount = 0;
int packetDivs = 1;

myPacket *server_packet;
myPacket *client_packet;
myPacket *temp_packet;

myPacket *packetStack;
int stackSize = 0;

char *inputBuffer;
std::vector<myPacket> receivedPacketVec;
std::vector<unsigned int> receivedAckVec;

//----------------Utilities--------------------

int clipper(char (&c_content)[11], char *inputBuffer, int start, int end){
    for(int i = start ; i < end ; ++i){
        c_content[i%11] = inputBuffer[i];
    }
    return 0;
}

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

bool intSearchBuffer(unsigned int search_id){
    for(myPacket i : receivedPacketVec){
        if(i.id ==search_id){
            *temp_packet = i;
            return true;
        }
    }
    return false;
}

bool eraseWithID(unsigned int erase_id){
    for (auto i=receivedPacketVec.begin(); i != receivedPacketVec.end(); i++){
        if((*i).id==erase_id){receivedPacketVec.erase(i,i);}
        return false;
    }
    return false;
}

//----------------Code_Starts_Here--------------------

int begin(){
    server_packet = (myPacket*) std::malloc(sizeof(myPacket));
    client_packet = (myPacket*) std::malloc(sizeof(myPacket));
    temp_packet = (myPacket*) std::malloc(sizeof(myPacket));
    inputBuffer = (char*) std::malloc(sizeof(char)*1034); // it is 1034 and not 1024 becuse it is divisible by 11

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

void listen_for_packets(){
    if(!intSearchBuffer(client_packet->id)){receivedPacketVec.push_back(*client_packet);} // if packet never received before save it
    if(intSearchBuffer(R_lastValidID)){ // read the saved packet buffer in order
        printf("%s",  temp_packet->content); // print the next buffer in line
        eraseWithID(R_lastValidID); //erase the already printed packet prom the buffer
        ++R_lastValidID;
        if(!strcmp(temp_packet->content,"\n")){
            ++newLineCount;
        }
        else{
            newLineCount = 0;
        }
        if(newLineCount == 3){ //termination condition
            pthread_cancel(send_ack_t);
            terminate = true;
        }
    }
}

void listen_for_acks(){
    ;
}

void* listen_routine(void* args){
    while(1){
        if(wait_ack_send){continue;} //wait for send
        if (recvfrom(socket_desc,  client_packet, packetsize, 0,
            (struct sockaddr*)&client_addr, &client_struct_length) < 0){
            perror("Couldn't receive\n");
            exit(EXIT_FAILURE);
        }
        listen_for_acks();
        listen_for_packets();
        if(terminate){return nullptr;} //if termination signal is given
        wait_ack_listen = false;
        wait_ack_send = true;
    }
    return nullptr;
}

void* send_routine(void* args){
    while(!terminate){
        if(wait_ack_listen){continue;} //wait for send
        server_packet = client_packet;
        
        if (sendto(socket_desc, server_packet, packetsize, 0,
            (struct sockaddr*)&client_addr, client_struct_length) < 0){
            perror("Can't send\n");
            exit(EXIT_FAILURE);
        }
        wait_ack_listen = true;
        wait_ack_send = false;
    }
    return nullptr;
}

int main(int argc, char** argv){
    begin();
    setSocket();
    pthread_create(&listen_t, nullptr, &listen_routine, nullptr);
    pthread_create(&send_ack_t, nullptr, &send_routine, nullptr);

    pthread_join(listen_t, nullptr);
    pthread_join(send_ack_t, nullptr);
}
