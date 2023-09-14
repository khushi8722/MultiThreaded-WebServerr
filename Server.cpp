#include <bits/stdc++.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>

using namespace std;

#define SERVER_PORT 8081
#define BUFSIZE 4096
#define SOCKETERROR -1
#define SERVER_BACKLOG 100 //maximum number of clients waiting for accepting connection


typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;


void * handle_connection(void * p_client_socket);
int check(int exp, const char *msg);

int main(int argc, char **argv){

    int server_socket, client_socket , addr_size ; 
    SA_IN server_addr, client_addr;

    check((server_socket = socket(AF_INET , SOCK_STREAM , 0)), "Failed to create Socket");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    check(bind(server_socket,(SA*)&server_addr, sizeof(server_addr)),"Bind Failed");

    check(listen(server_socket, SERVER_BACKLOG), "Listen Failed!!");


    while(1){
        cout<<"Waiting for connections...\n";
        // wait for connections

        addr_size = sizeof(SA_IN);
        check((client_socket = accept(server_socket,(SA*)&client_addr, (socklen_t*)&addr_size)),"Accept Failed");

        cout<<"CONNECTED.\n";

        //now server is ready to handle request by the client
        pthread_t t ;
        int *pclient = (int*)malloc(sizeof(int));
        *pclient = client_socket;
        pthread_create(&t,NULL,handle_connection,pclient);
        //handle_connection(pclient);
    }

    return 0;
}

int check(int exp, const char *msg){
    if(exp == SOCKETERROR){
        cout<<msg<<"\n";
        exit(1);
    }
    return exp;
}

void  * handle_connection(void * p_client_socket) {
    int client_socket = *((int *)p_client_socket);
    free(p_client_socket);
    char buffer[BUFSIZE];
    size_t bytes_read;
    int msgsize = 0;
    char actualpath[PATH_MAX+1];

    while((bytes_read = read(client_socket,buffer+msgsize, sizeof(buffer)-msgsize-1))>0) {
        msgsize += bytes_read;
        if(msgsize > BUFSIZE-1 || buffer[msgsize-1] == '\n') break;
    }

    check(bytes_read,"received error");

    buffer[msgsize-1] = 0;

    cout<<"Request: "<<buffer<<"\n";

    if(realpath(buffer, actualpath) == NULL) {
        cout<<"Error(bad path) : "<<buffer<<"\n"<<flush;
        close(client_socket);
        return NULL;
    }

    FILE *fp = fopen(actualpath, "r");

    if(fp == NULL) {
        cout<<"Error(open) : "<<buffer<<"\n";
        close(client_socket);
        return NULL;
    }

    while((bytes_read = fread(buffer, 1, BUFSIZE, fp))>0){
        cout<<"Sending "<<bytes_read<<" bytes\n";
        write(client_socket, buffer, bytes_read);
    }
    sleep(1);
    close(client_socket);
    fclose(fp);
    cout<<"Closing Connection\n";
    return NULL;
}