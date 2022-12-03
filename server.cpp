#include "common.h"
#include <cstdio>
#include <pthread.h>
#include <string.h>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080

pthread_t listen_t;
pthread_t send_t;
size_t buffersize = 1024;
char* buffer;
bool terminate = false;
int opt = 1;
struct sockaddr_in address;
int addrlen = sizeof(address);
int sockfd ,new_socket;

void* listen_routine(void* args){
    while(!terminate){
        ;
    }
    return nullptr;
}

void* send_routine(void* args){
    while(!terminate){
        ;
    }
    return nullptr;
}

int setSocket(){
    int valread;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Creating socket file descriptor
    setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); // Forcefully attaching socket to the port 8080

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; //determine the adress
    address.sin_port = htons(PORT); //determine the adress

    bind(sockfd, (struct sockaddr*)&address,sizeof(address)); // Forcefully attaching socket to the port 8080
    new_socket= accept(sockfd, (struct sockaddr*)&address,(socklen_t*)&addrlen);

    valread = read(new_socket, buffer, 1024);
    printf("%s\n", buffer);
    send(new_socket, "hello", strlen("hello"), 0);
    printf("Hello message sent\n");


    return 0;
}

int loop(){
    int newLineCount = 0;
    while(1){
        getline(&buffer, &buffersize ,stdin);
        if(!strcmp(buffer,"\n")){
            ++newLineCount;
        }
        else{
            newLineCount = 0;
        }
        if(newLineCount == 3){ //termination condition
            terminate = true;
            close(new_socket); // closing the connected socket
            shutdown(sockfd, SHUT_RDWR); // closing the listening socket
            return 0;
        }
        buffer[strlen(buffer)-1] = '\0';
    }
}

int main(void){
    //printf("sizeof: myAck: %ld bytes\n",sizeof(myAck));
    //printf("sizeof: myPacket: %ld bytes\n",sizeof(myPacket));
    buffer = (char*) std::malloc(buffersize*sizeof(char));

    pthread_create(&listen_t, nullptr, &listen_routine, nullptr);
    pthread_create(&send_t, nullptr, &send_routine, nullptr);

    setSocket();
    loop();

    pthread_join(listen_t, nullptr);
    pthread_join(send_t, nullptr);
    return 0;
}
