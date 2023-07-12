#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/wait.h>

pthread_mutex_t lock;

//FOR COUNTING CLIENT
int client=0;

//CLIENT SOCKET FILE DESCRIPTOR
int *clientfd;

pthread_t client_thread;
pthread_t client_read,client_write;

//FOR ERROR HANDLING
void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

//CLIENT STDIN TO SERVER
void *clientread(void*arg){
    //FOR EXIT THREAD WHEN PROGRAM IS DONE
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    

    int *pipe_stdin=(int*)arg;
    char *buffer;

    while(1){
        buffer=(char*)malloc(sizeof(char)*1024);
        size_t read_len=recv(*clientfd,buffer,sizeof(buffer),0);//RECEIVE CLIENT'S STDIN
        buffer[read_len]='\0';
        write(pipe_stdin[1],buffer,strlen(buffer));//WRITE CMD TO PROGRAM
        free(buffer);
    }
    pthread_exit(NULL);
    return NULL;

}

//SERVER PROGRAM STDOUT TO CLIENT
void * clientwrite(void*arg){
    //FOR EXIT THREAD WHEN PROGRAM IS DONE
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    

    int *pipe_stdout=(int*)arg;
    char buffer[1024];
    ssize_t bytesRead;
    
    //FD to file stream
    FILE* output = fdopen(pipe_stdout[0], "r");
        if (output == NULL) {
            perror("Error in reading stdout pipe");
            exit(1);
        }

        //READ STDOUT to SEND CLIENT
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), output)) > 0) {
           
            buffer[bytesRead]='\0';
            if (send(*clientfd, buffer, strlen(buffer), 0) < 0) {
                perror("Error in sending output");
                exit(1);
            }
        }
        pthread_exit(NULL);
        return NULL;

}


//AFTER CONNECT, handling the client
void *handle_client(void *arg) {
    int client_fd = *((int *)arg);

    // For thread
    char filename[64];
    char progname[64];

    //FOR SETTING Received file name and output filename
    int tid=(int)client_thread;
    snprintf(filename,sizeof(filename),"received_file_%d.c",tid);
    snprintf(progname,sizeof(progname),"output_%d",tid);


    //RECEIVE FILE SIZE FIRST
    char fsize[5];
    recv(client_fd,fsize,sizeof(fsize),0);
    int size=atoi(fsize);

    //RECEIVE FILE FROM CLIENT
    FILE *file = fopen(filename, "wb");
    char buffer[256];
    int nbyte=1024;
    int cur=0;
    while (cur<size) {
        nbyte = recv(client_fd, buffer, sizeof(size),0);
        cur+=nbyte;
        fwrite(buffer, 1, nbyte, file);
    }
    fclose(file);


    // COMPILE THE RECEVIED FILE USING GCC
    char compilestr[256];
    snprintf(compilestr,sizeof(compilestr),"gcc %s -o %s",filename,progname);
    system(compilestr);


    //SETTING CMD FOR EXECUTING PROGRAM
    char programstr[256];
    snprintf(programstr,sizeof(programstr),"./%s",progname);

    //MAKE 2 PIPE FOR BIDIRECTIONAL COMMUNICATION
    //REDIRECTION OF STDOUT AND STDIN
    int pipe_stdin[2];
    int pipe_stdout[2];
    pipe(pipe_stdin);
    pipe(pipe_stdout);


    //FORK()
    pid_t pid=fork();
    if(pid<0){
        printf("fork() error\n");
    }
    else if(pid==0){//CHILD PROCESS-> EXECUTING PROGRAM
        close(pipe_stdout[0]);
        dup2(pipe_stdout[1],STDOUT_FILENO); 
        close(pipe_stdout[1]);

        close(pipe_stdin[1]);//
        dup2(pipe_stdin[0],STDIN_FILENO);
        close(pipe_stdin[0]);

        //EXECUTING PROGRAM 
        execl(programstr,programstr,NULL);
    }
    else{//PARENT PROCESS
        close(pipe_stdout[1]);
        close(pipe_stdin[0]);

        //CREATE THREAD FOR THE PROGRAM
        if (pthread_create(&client_read, NULL, clientread, pipe_stdin)!=0) {
            error_handling("pthread_create() error");
        }
        if (pthread_create(&client_write, NULL, clientwrite, pipe_stdout)!=0) {
            error_handling("pthread_create() error");
        }

        pthread_detach(client_read);
        pthread_detach(client_write);

        //WAITING FOR THE PROGRAM TERMINATION
        waitpid(pid,NULL,0);


        //AFTER PROGRAM TERMINATION, KILL THREADS
        pthread_cancel(client_read);
        pthread_cancel(client_write);
        close(*clientfd);

    }
    close(pipe_stdin[0]);
    close(pipe_stdin[1]);
    close(pipe_stdout[0]);
    close(pipe_stdout[1]);

    printf("[*] session closed\n");
    free(arg);
    pthread_mutex_lock(&lock);
    client--;
    pthread_mutex_unlock(&lock);
    return NULL;
}






int main(int argc, char *argv[]) {

    //NEED 1 ARGUMENT : PORT
    if(argc!=2){
        printf("Usage : %s <port>\n",argv[0]);
    }


    //SETUP SERVER SOCKET
    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1){
        error_handling("socket() error");
    }
    
    struct sockaddr_in serv_addr; 
    memset(&serv_addr, 0 , sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    if(bind(listen_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1){
        error_handling("bind() error");
    }
    if(listen(listen_fd,SOMAXCONN)==-1){
        error_handling("listen() error");
    }


    //WAITING AND HANDLING CLIENT CONTINUOSLY
    while(1){
        if(client<=0){
            printf("[*] wait for client ...\n");//WAITING FOR CLIENT
        }
        struct sockaddr_in clientaddr;
        socklen_t clientaddrlen=sizeof(clientaddr);
        clientfd=(int *)malloc(sizeof(int));
        *clientfd=accept(listen_fd,(struct sockaddr *)&clientaddr,&clientaddrlen);
        printf("[*] client connected\n");
        client++;

        //CREATE THREAD TO HANDLE CLIENT
        if (pthread_create(&client_thread, NULL, handle_client, clientfd)!=0) {
            error_handling("pthread_create() error");
            free(clientfd);
            continue;
        }
        pthread_join(client_thread,NULL);
    }

    close(listen_fd);

    return(0);
}


