#include "common.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <pthread.h>
#include <cstdlib>
#define PORT 2222

pthread_t listen_t;
pthread_t send_t;
size_t buffersize = 1024;
char* buffer;
bool terminate = false;
bool new_sent_message = false;

int server_fd, new_socket, valread;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);
char listen_buffer[1024] = { 0 };
char* hello = "Hello from server";

int setSocket(){
 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
 
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address,sizeof(address))< 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket= accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen))< 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    return 0;
}

void* listen_routine(void* args){
    while(!terminate){
        valread = read(new_socket, listen_buffer, 1024);
        printf("%s\n", listen_buffer);
    }
    return nullptr;
}

void* send_routine(void* args){
    while(!terminate){
        if(new_sent_message){
            new_sent_message = false;
            send(new_socket, hello, strlen(hello), 0);
            printf("Hello message sent\n");
        }
    }
    return nullptr;
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
            close(new_socket);
            shutdown(server_fd, SHUT_RDWR);
            return 0;
        }
        buffer[strlen(buffer)-1] = '\0';
        new_sent_message = true;
    }
}

int main(void){
    buffer = (char*) std::malloc(buffersize*sizeof(char));

    setSocket();
    pthread_create(&listen_t, nullptr, &listen_routine, nullptr);
    pthread_create(&send_t, nullptr, &send_routine, nullptr);

    loop();

    pthread_join(listen_t, nullptr);
    pthread_join(send_t, nullptr);
    return 0;
}
