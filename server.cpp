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

int socket_desc_1;
int socket_desc_2;
struct sockaddr_in server_addr_1, client_addr_1;
struct sockaddr_in server_addr_2;

//----------------Some_Constants--------------------

unsigned int windowSize = 8;
size_t packetsize = 16;
size_t contentsize = 11;
int port = 2222;
socklen_t client_struct_length = sizeof(client_addr_1);
socklen_t server_struct_length = sizeof(server_addr_1);
float timeoutDuration = 0.211;
size_t inputBufferSize = 1034; // it is 1034 and not 1024 becuse it is divisible by 11

//----------------Global_Variables--------------------

pthread_t listen_ack_t,listen_packet_t,send_packet_t,send_ack_t,backn_t;

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

myPacket *server_packet_1;
myPacket *client_packet_1;

myPacket *server_packet_2;
myPacket *client_packet_2;
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
    server_packet_1 = (myPacket*) std::malloc(sizeof(myPacket));
    client_packet_1 = (myPacket*) std::malloc(sizeof(myPacket));

    server_packet_2 = (myPacket*) std::malloc(sizeof(myPacket));
    client_packet_2 = (myPacket*) std::malloc(sizeof(myPacket));
    packetStack = (myPacket*) std::malloc(sizeof(myPacket)*windowSize); //keep as many packets as window size
    temp_packet = (myPacket*) std::malloc(sizeof(myPacket));
    inputBuffer = (char*) std::malloc(sizeof(char)*1034); // it is 1034 and not 1024 becuse it is divisible by 11

    return 0;
}

int setSocket_1(){
    socket_desc_1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Generate the Socket
    
    if(socket_desc_1 < 0){
        perror("Error while creating socket\n");
        exit(EXIT_FAILURE);
    }
    
    server_addr_1.sin_family = AF_INET;
    server_addr_1.sin_port = htons(port); //set the port
    server_addr_1.sin_addr.s_addr = inet_addr("127.0.0.1"); //set the IP's
    
    if(bind(socket_desc_1, (struct sockaddr*)&server_addr_1, sizeof(server_addr_1)) < 0){ // make the binding for setting port and IP
        perror("Couldn't bind to the port\n");
        exit(EXIT_FAILURE);
    }
    return 0;    
}

int setSocket_2(){
    socket_desc_2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Generate the Socket
    
    if(socket_desc_2 < 0){
        perror("Error while creating socket\n");
        exit(EXIT_FAILURE);
    }
    
    server_addr_2.sin_family = AF_INET;
    server_addr_2.sin_port = htons(port+1); //set the port
    server_addr_2.sin_addr.s_addr =  inet_addr("127.0.0.1"); //set the IP
    
    return 0;
}

void* listen_packet_routine(void* args){
    while(1){
        if(wait_ack_send){continue;} //wait for send
        if (recvfrom(socket_desc_1,  client_packet_1, packetsize, 0,
            (struct sockaddr*)&client_addr_1, &client_struct_length) < 0){
            perror("Couldn't receive\n");
            exit(EXIT_FAILURE);
        }
        if(!intSearchBuffer(client_packet_1->id)){receivedPacketVec.push_back(*client_packet_1);} // if packet never received before save it
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
                pthread_cancel(send_packet_t);
                pthread_cancel(listen_ack_t);
                //pthread_cancel(backn_t);
                terminate = true;
                return nullptr;
            }
        }
        if(terminate){return nullptr;} //if termination signal is given
        wait_ack_listen = false;
        wait_ack_send = true;
    }
    return nullptr;
}

void* listen_ack_routine(void* args){
    while(1){
        if(recvfrom(socket_desc_2, server_packet_2, sizeof(server_packet_2), 0,
            (struct sockaddr*)&server_addr_2, &server_struct_length) < 0){
            perror("Error while receiving server's msg\n");
            exit(EXIT_FAILURE);
            return nullptr;
        }
        if(server_packet_2->id >= windowTail){ //if it is a significant package
            if(!std::count(receivedAckVec.begin(), receivedAckVec.end(), server_packet_2->id)){receivedAckVec.push_back(server_packet_2->id);} //if first time save ack
            if(windowTail == server_packet_2->id || std::count(receivedAckVec.begin(), receivedAckVec.end(), windowTail)){ //check if tail ack is received
                receivedAckVec.erase(std::remove(receivedAckVec.begin(), receivedAckVec.end(), windowTail), receivedAckVec.end()); //delete old tail
                windowTail = windowTail + 1; //the last received Ack is pushed out of the window
                windowHead = windowTail + windowSize - 1; //the next packet is added to the window and it is now sendable
                tailTimeout = std::chrono::system_clock::now(); //reset the timeout value to now
            }
        }
        printf("[tail: %d| head: %d] Ack Number: %d \n",windowTail,windowHead, server_packet_2->id);
    }
    return nullptr;
}

void* send_ack_routine(void* args){
    while(!terminate){
        if(wait_ack_listen){continue;} //wait for send
        server_packet_1 = client_packet_1;
        
        if (sendto(socket_desc_1, server_packet_1, packetsize, 0,
            (struct sockaddr*)&client_addr_1, client_struct_length) < 0){
            perror("Can't send\n");
            exit(EXIT_FAILURE);
        }
        wait_ack_listen = true;
        wait_ack_send = false;
    }
    return nullptr;
}

void* send_packet_routine(void* args){
    while(1){
        getline(&inputBuffer, &inputBufferSize ,stdin);
        packetDivs = ((strlen(inputBuffer)/contentsize)+1);

        for(int i = 0 ; i<packetDivs; ++i){
            while(S_lastValidID >= windowHead){continue;} //only sending packets up to window head
            ++S_lastValidID;
            client_packet_2->id = S_lastValidID;
            clipper(client_packet_2->content,inputBuffer,i*contentsize,(i+1)*contentsize);
            packetStackManager(*client_packet_2); //add the to be sent package to the window package stack
            if(sendto(socket_desc_2, client_packet_2, packetsize, 0,
                (struct sockaddr*)&server_addr_2, server_struct_length) < 0){
                perror("Unable to send message\n");
                exit(EXIT_FAILURE);
            }
            tailTimeout = std::chrono::system_clock::now(); //set the timer if a package is sent
        }
        if(!strcmp(inputBuffer,"\n")){
            ++newLineCount;
        }
        else{
            newLineCount = 0;
        }
        if(newLineCount == 3){ //termination condition
            pthread_cancel(send_ack_t);
            pthread_cancel(listen_packet_t);
            pthread_cancel(listen_ack_t);
            //pthread_cancel(backn_t);
            terminate = true;
            return nullptr;
        }
    }
}

int main(int argc, char** argv){
    begin();
    setSocket_1();
    setSocket_2();
    pthread_create(&listen_packet_t, nullptr, &listen_packet_routine, nullptr);
    pthread_create(&send_ack_t, nullptr, &send_ack_routine, nullptr);

    pthread_create(&listen_ack_t, nullptr, &listen_ack_routine, nullptr);
    pthread_create(&send_packet_t, nullptr, &send_packet_routine, nullptr);
    //pthread_create(&backn_t, nullptr, &backn_routine, nullptr);

    //pthread_join(backn_t, nullptr);
    pthread_join(listen_ack_t, nullptr);
    pthread_join(send_packet_t, nullptr);

    pthread_join(listen_packet_t, nullptr);
    pthread_join(send_ack_t, nullptr);

    close(socket_desc_1);
    close(socket_desc_2);
}
