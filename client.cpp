#include "common.h"
#include <cstdio>
#include <pthread.h>
#include <string.h>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 2222

pthread_t listen_t;
pthread_t send_t;
size_t buffersize = 1024;
char* buffer;
bool terminate = false;
bool new_sent_message = false;

int sock = 0, valread, client_fd;
struct sockaddr_in serv_addr;
char* hello = "Hello from client";
char listen_buffer[1024] = { 0 };
 
int setSocket(){
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
 
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
 
    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr,sizeof(serv_addr)))< 0) {
        printf("\nConnection Failed \n");
        return -1;
    } 
    return 0;
}

void* listen_routine(void* args){
    while(!terminate){
        valread = read(sock, listen_buffer, 1024);
        printf("%s\n", listen_buffer);;
    }
    return nullptr;
}

void* send_routine(void* args){
    while(!terminate){
        if(new_sent_message){
            new_sent_message = false;
            send(sock, buffer, strlen(buffer), 0);
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
            close(client_fd);
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
