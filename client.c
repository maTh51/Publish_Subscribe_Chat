#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 500

int *flag;

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server Port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511", argv[0]);
    exit(EXIT_FAILURE);
}

void * rec_thread(void *arg) {
    int *s = (int *)arg;
    char bufr[BUFSZ];
    unsigned total;
    while (1) {
        memset(bufr, 0, BUFSZ);
        total = 0;
        while (1) {
            size_t count = recv(*s, bufr + total, BUFSZ - total, 0);           
            if(count == 0){  // Connection terminated  
                printf("\33[2K\r");
                printf("[log] received 0 bytes\n>> Connection closed by Server\n");
                *flag = 1;
                pthread_exit(EXIT_SUCCESS);    
            }
            total += count;

            if( bufr[(int)total-1] == '\n')    //mensagem
                break;          
            
        }

        printf("\33[2K\r");
        printf("[log] received %u bytes\n>> ", total);
        for(int i=0; i<strlen(bufr); i++){
            if(check_ch(bufr[i]) == 0){
                printf("Message with proibided chars. Bug server. Closing connection...");
                close(*s);
                *flag = 1;
                pthread_exit(EXIT_SUCCESS);
            }
        }
        puts(bufr);
        printf("> ");
        fflush(stdout);
        if(total == 0)
            break;
    }

    return(EXIT_SUCCESS);
    
}

int main(int argc, char **argv) {
    if(argc < 3)
        usage(argc, argv);

    struct sockaddr_storage storage;
    if(0 != addrparse(argv[1], argv[2], &storage))
        usage(argc, argv);

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1) 
        logexit("socket");

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(0 != connect(s, addr, sizeof(storage)))
        logexit("connect");

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    printf("connected to %s\n", addrstr);   

    //Thread to recv
    pthread_t rect;
    flag = (int *)malloc(sizeof(int));
    *flag = 0;  //para comunicar entre a thread e o principal
    pthread_create(&rect, NULL, rec_thread, &s);  
    while(*flag == 0) {
        
        char bufw[BUFSZ];
        memset(bufw, 0, BUFSZ);
        printf("> ");
        fgets(bufw, BUFSZ, stdin);
        
        size_t count = send(s, bufw, strlen(bufw), 0);
        if(count != strlen(bufw))
            logexit("send");
        
    }
    
    exit(EXIT_SUCCESS);
}