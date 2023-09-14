#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
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

#define SERVER_PORT 8080
#define BUFSIZE 4096
#define SOCKETERROR -1
#define SERVER_BACKLOG 100 //maximum number of clients waiting for accepting connection
#define THREAD_POOL_SIZE 20 //number of threads that are going to handle requests

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
queue<int*> requests;
char http_header[25] = "HTTP/1.1 200 Ok\r\n";

void * handle_connection(void * p_client_socket);
int check(int exp, const char *msg);
void * thread_function(void * arg);
char* parse(char line[], const char symbol[]);
char* parse_method(char line[], const char symbol[]);
//char* find_token(char line[], const char symbol[], const char match[]);
int send_message(int fd, char image_path[], char head[]);
//void processing_request(char buffer[], char** http_method, char** request_filepath);

int main(int argc, char **argv){
    int server_socket, client_socket , addr_size ; 
    SA_IN server_addr, client_addr;


    for (int i=0; i < THREAD_POOL_SIZE; ++i) {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }

    check((server_socket = socket(AF_INET , SOCK_STREAM , 0)), "Failed to create Socket");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;//0.0.0.0
    server_addr.sin_port = htons(SERVER_PORT);

    check(bind(server_socket,(SA*)&server_addr, sizeof(server_addr)),"Bind Failed");

    check(listen(server_socket, SERVER_BACKLOG), "Listen Failed!!");

    while(1){
        cout<<"Waiting for connections...\n";
        // wait for connections

        addr_size = sizeof(SA_IN);

        check((client_socket = accept(server_socket,(SA*)&client_addr, (socklen_t*)&addr_size)),"Accept Failed");

        cout<<"CONNECTED.\n";
        
        int * pclient = (int *)malloc(sizeof(int));
        *pclient = client_socket;
        pthread_mutex_lock(&lock1);
        requests.push(pclient);//accessing a common Data Structure(NOT SAFE)
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock1);


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

void * thread_function(void * arg) {
    while(true) {
        //if(!requests.empty()){
        int *pclient ;
        bool bl = 0;
        pthread_mutex_lock(&lock1);
        bl = requests.empty();
        if(!bl){
            pclient = requests.front();
            requests.pop();
        }else{
            pthread_cond_wait(&cond,&lock1);
        }
        pthread_mutex_unlock(&lock1);
        if(!bl)
            handle_connection(pclient);
            //we have a connection
        
    }
}


void *  handle_connection(void *p_client_socket) {

    int client_socket = *((int *)p_client_socket);
    free(p_client_socket);

    //pthread_mutex_lock(&lock2);

    char buffer[30000] ;
    
    long valread = read(client_socket, buffer, 30000);//client request
    //cout<<buffer<<"\n";
    //Parsing the method used by the HTTP
    char *parse_string_method = parse_method(buffer, " ");  
    cout<<"HTTP Method:"<<parse_string_method<<"\n";
    //Parsing the file requested by the browser
    char *parse_string = parse(buffer, " ");
    //This is not the exac path, exact path depends upon filetype
    cout<<"Path for the file:"<<parse_string<<"\n";
    
    //+1 size is allocated for '/0'
    char *copy = (char *)malloc(strlen(parse_string) + 1);
    strcpy(copy, parse_string);
    //Parsing the extension of the file asked
    char *parse_ext = parse(copy, ".");  // get the file extension such as JPG, jpg

    char *copy_head = (char *)malloc(strlen(http_header) +200);//Extra space for storing Content-Type
    strcpy(copy_head, http_header);
    
    if(parse_string_method[0] == 'G' && parse_string_method[1] == 'E' && parse_string_method[2] == 'T'){

        if(strlen(parse_string) <= 1){
             //parse_string=" "or"/"--> Send index.html file
            char path_head[500] = ".";
            strcat(path_head, "/htmlfiles/index.html");
            strcat(copy_head, "Content-Type: text/html\r\n\r\n");
            //cout<<"\n\n\n"<<path_head<<"\n\n\n";
            send_message(client_socket, path_head, copy_head);
        }
        else if ((parse_ext[0] == 'h' && parse_ext[1] == 't' && parse_ext[2] == 'm' && parse_ext[3] == 'l') || (parse_ext[0] == 'J' && parse_ext[1] == 'P' && parse_ext[2] == 'G'))
        {
            //send html file to client
            char path_head[500] = "./";
            strcat(path_head,"htmlfiles");
            strcat(path_head, parse_string);
            //cout<<"\n\n\n"<<path_head<<"\n\n\n";
            strcat(copy_head, "Content-Type: text/html\r\n\r\n");
            send_message(client_socket, path_head, copy_head);
        }
        else if ((parse_ext[0] == 'j' && parse_ext[1] == 'p' && parse_ext[2] == 'g') || (parse_ext[0] == 'J' && parse_ext[1] == 'P' && parse_ext[2] == 'G'))
        {
            //send image to client(jpg,JPG)
            char path_head[500] = "./";
            strcat(path_head, "img");
            strcat(path_head, parse_string);
            strcat(copy_head, "Content-Type: image/jpeg\r\n\r\n");
            send_message(client_socket, path_head, copy_head);
        }
        else if (parse_ext[0] == 'p' && parse_ext[1] == 'n' && parse_ext[2] == 'g')
        {
            char path_head[500] = "./";
            strcat(path_head, "img");
            strcat(path_head, parse_string);
            strcat(copy_head, "Content-Type: image/png\r\n\r\n");
            send_message(client_socket, path_head, copy_head);
        }
        else if (parse_ext[0] == 'i' && parse_ext[1] == 'c' && parse_ext[2] == 'o')
        {
            //ico(icons)
            char path_head[500] = ".";
            strcat(path_head, "/img/favicon.png");
            strcat(copy_head, "Content-Type: image/vnd.microsoft.icon\r\n\r\n");
            send_message(client_socket, path_head, copy_head);
        }
        else if (parse_ext[strlen(parse_ext)-2] == 'j' && parse_ext[strlen(parse_ext)-1] == 's')
        {
            //javascript
            char path_head[500] = ".";
            strcat(path_head, parse_string);
            strcat(copy_head, "Content-Type: text/javascript\r\n\r\n");
            send_message(client_socket, path_head, copy_head);
        }
        else if (parse_ext[strlen(parse_ext)-3] == 'c' && parse_ext[strlen(parse_ext)-2] == 's' && parse_ext[strlen(parse_ext)-1] == 's')
        {
            //css
            char path_head[500] = ".";
            strcat(path_head, parse_string);
            strcat(copy_head, "Content-Type: text/css\r\n\r\n");
            send_message(client_socket, path_head, copy_head);
        }
        else{
            //send other file 
            char path_head[500] = ".";
            strcat(path_head, parse_string);
            strcat(copy_head, "Content-Type: text/plain\r\n\r\n");
            send_message(client_socket, path_head, copy_head);
        }
            cout<<"\n------------------Request Processed--------------------\n";
    }else if (parse_string_method[0] == 'P' && parse_string_method[1] == 'O' && parse_string_method[2] == 'S' && parse_string_method[3] == 'T'){
        //Handle POST Requests
    }
    //pthread_mutex_unlock(&lock2);
        close(client_socket);//since the request type is mainly HTTP/1.1 we should not be closing this connection until we have fewer clients(less than the number of threads)
        free(copy);
        free(copy_head);
    }


char* parse(char line[], const char symbol[])
{
    char *copy = (char *)malloc(strlen(line) + 1);
    strcpy(copy, line);
    
    char *message=(char*)"";//uninitialized char pointers cause segmentation fault error
    char * token = strtok(copy, symbol);
    int current = 0;

    while( token != NULL ) {
      
      token = strtok(NULL, " ");
      if(current == 0){
          message = token;
          if(message == NULL){
              message = (char*)"";
          }
          return message;
      }
      current = current + 1;
   }
   free(token);
   free(copy);
   return message;
}

char* parse_method(char line[], const char symbol[])
{
    char *copy = (char *)malloc(strlen(line) + 1);
    strcpy(copy, line);
        
    char *message=(char*)"";
    char * token = strtok(copy, symbol);
    int current = 0;

    while( token != NULL ) {
      
      //token = strtok(NULL, " ");
      if(current == 0){
          message = token;
          if(message == NULL){
              message =(char*) "";
          }
          return message;
      }
      current = current + 1;
   }
   free(copy);
   free(token);
   return message;
}

int send_message(int fd, char file_path[], char head[]){
    //pthread_mutex_lock(&lock2);
    int fdimg = open(file_path, O_RDONLY);
    //pthread_mutex_unlock(&lock2);
    
    if(fdimg < 0){
        printf("Cannot Open file path : %s with error %d\n", file_path, fdimg);
        //if requested file is not found display a page showing causes of error
        file_path = (char*)"./htmlfiles/Error.html\0";
        head = (char*)"HTTP/1.1 200 Ok\r\nContent-Type: text/html\r\n\r\n";
        fdimg = open(file_path, O_RDONLY);

    }

    struct stat stat_buf;  /* hold information about input file */

    write(fd, head, strlen(head));

    fstat(fdimg, &stat_buf);
    int img_total_size = stat_buf.st_size;
    int block_size = stat_buf.st_blksize;

    if(fdimg >= 0){
        ssize_t sent_size;

        while(img_total_size > 0){
              int send_bytes = ((img_total_size < block_size) ? img_total_size : block_size);
              int done_bytes = sendfile(fd, fdimg, NULL, block_size);
              img_total_size = img_total_size - done_bytes;
        }
        if(sent_size >= 0){
            printf("send file: %s \n" , file_path);
        }
        close(fdimg);
    }
}

// void  * handle_connection(void * p_client_socket) {
//     int client_socket = *((int *)p_client_socket);
//     free(p_client_socket);
//     char buffer[BUFSIZE];
//     size_t bytes_read;
//     int msgsize = 0;
//     char actualpath[PATH_MAX+1];

//     while((bytes_read = read(client_socket,buffer+msgsize, sizeof(buffer)-msgsize-1))>0) {
//         msgsize += bytes_read;
//         if(msgsize > BUFSIZE-1 || buffer[msgsize-1] == '\n') break;
//     }
//     cout<<bytes_read<<"\n";
//     check(bytes_read,"recv error");

//     buffer[msgsize-1] = 0;

//     cout<<"Request: "<<buffer<<"\n";

//     if(realpath(buffer, actualpath) == NULL) {
//         cout<<"Error(bad path) : "<<buffer<<"\n"<<flush;
//         close(client_socket);
//         return NULL;
//     }

//     FILE *fp = fopen(actualpath, "r");

//     if(fp == NULL) {
//         cout<<"Error(open) : "<<buffer<<"\n";
//         close(client_socket);
//         return NULL;
//     }

//     while((bytes_read = fread(buffer, 1, BUFSIZE, fp))>0){
//         cout<<"Sending "<<bytes_read<<" bytes\n";
//         write(client_socket, buffer, bytes_read);
//     }
//     sleep(1);
//     close(client_socket);
//     fclose(fp);
//     cout<<"Closing Connection\n";
//     return NULL;
// }