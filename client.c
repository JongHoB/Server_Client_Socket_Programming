#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

//socket file descriptor
int sockfd;

pthread_t server_read,server_write;

//For error handling
void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

// SERVER PROGRAM STDOUT TO Client
void *serverread(void* arg){

    char*buffer;//

    while(1){

        buffer=(char*)malloc(sizeof(char)*1024);

        //Receive data from Server
        size_t read_len=recv(sockfd,buffer,sizeof(buffer),0);

        if(read_len==-1){
           error_handling("read() error");
           break; 
        }

        // Server Program 종료시 END.
        if(read_len==0){
            printf("[*] session closed\n");
            break;
        }

        buffer[read_len]='\0';

        //STD
        printf("%s",buffer);
        free(buffer);
    }
    close(sockfd);
    return NULL;
}

// Client STDIN To SERVER PROGRAM
void *serverwrite(void*arg){
    //FOR EXIT Thread WHEN PROGRAM IS DONE
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
 

    char input_buffer[1024];

    //READ FROM Client STDIN
    while(1){
        if(fgets(input_buffer, sizeof(input_buffer), stdin)!=NULL)
           { 
            //SEND THE DATA TO SERVER
            send(sockfd, input_buffer, strlen(input_buffer),0);}
        else{
            if(feof(stdin)){
                pthread_exit(NULL);
            }
            break;
        }
    }
    return NULL;
}



int main(int argc, char *argv[]) {
    
    // NEED 3 Arguments: IP/PORT/FILEPATH
    if(argc!=4){
        printf("Usage : %s <IP> <port> <filepath>\n",argv[0]);
        exit(1);
    }

    //SETUP SOCKET using IPv4 and TCP
    //sockfd is file descriptor(return value from socket)
    sockfd=socket(PF_INET,SOCK_STREAM,0);
    if(sockfd==-1){
        error_handling("socket() error");
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    if(connect(sockfd,(const struct sockaddr *)&servaddr,sizeof(servaddr))==-1){
        error_handling("connect() error");
    }


    //FOR file transfer
    long int size;
    FILE *file=fopen(argv[3],"rb");
    if(file==NULL){
        error_handling("fopen() error");
    }
    else{//FOR CHECKING FILE SIZE
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fseek(file,0,SEEK_SET);
    }

    char fsize[5];
    sprintf(fsize,"%ld",size);
    send(sockfd,fsize,sizeof(fsize),0);//SEND FILE SIZE FIRST

    //SEND FILE TO SERVER
    char *buffer;
    buffer=(char*)malloc(sizeof(char)*size);
    while(!feof(file)){
        size_t fileread=fread(buffer,1,size,file);
        send(sockfd,buffer,fileread,0);
    }
    fclose(file);
    free(buffer);

    //CREATE THREAD FOR READ AND WRITE
    if (pthread_create(&server_read, NULL, serverread, NULL)!=0) {
            error_handling("pthread_create() error");
        }
    if (pthread_create(&server_write, NULL,serverwrite, NULL)!=0) {
            error_handling("pthread_create() error");
    }

    //WAITING FOR END FROM SERVER
    pthread_join(server_read,NULL);
    pthread_cancel(server_write);//AFTER END, EXITING THREAD
    pthread_detach(server_write);

    close(sockfd);
    return(0);
}




