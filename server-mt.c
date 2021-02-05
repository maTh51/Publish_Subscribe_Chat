#include "common.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFSZ 500
#define THREAD_POOL_SIZE 15

pthread_t tpool[THREAD_POOL_SIZE];
pthread_mutex_t mutex;

void usage(int argc, char **argv) {
    printf("usage: %s <server Port>\n", argv[0]);
    printf("example: %s 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};

tag_list *tags;

void * send_client_thread(void *data) {

    int *s = (int *)data;

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    unsigned total = 0;

    while(1) {
        size_t count = recv(*s, buf + total, BUFSZ - total, 0); 
        if((int)count == 0){
            printf("[log] client %i desconected (or with bug)\n", *s);
            close_cli(tags, *s);
            close(*s);
            return(data);
        }
        total += count;
        if( buf[(int)total-1] == '\n')  
            break;
    }
    printf("\n[msg] s: %i, %d bytes: %s", *s, (int)total, buf);

    total = 0;

    for(int i=0; i<strlen(buf); i++){
        if(check_ch(buf[i]) == 0){      //checar se contém chars invalidos
            printf("[log] client %i send invalid messages\n", *s);
            // char *msg_inv = "Message contains one or more characters not allowed on the server. You will be disconnected ...\n";
            // size_t count = send(*s, msg_inv, strlen(msg_inv), 0);
            // if (count != strlen(msg_inv)) 
            //     logexit("send");

            pthread_mutex_lock(&mutex);
            close_cli(tags, *s);
            pthread_mutex_unlock(&mutex);

            close(*s);
            return(data);
        }
    }
    
    char *n_token, *s_token;
    int tag_flag = 0;

    char *end_n;
    n_token = strtok_r(buf, "\n", &end_n); //pega cada msg que tenha um \n
    
    while( n_token != NULL ) {   
        
        if(strcmp(n_token, "##kill") == 0){   //caso kill
            printf("[log] client %i request ##kill. Shutting down server\n", *s);
            pthread_mutex_lock(&mutex);
            int_list *kill = criar_lc();
            kill = get_cli_bt_id(tags, "##kill");
            
            if(kill != NULL) {
                while (*kill != NULL) {
                    //size_t count = send((*kill)->s_client, "'##kill' command received. Shutting down server...\n", 45, 0);
                    //if (count != 45) 
                    //    logexit("send");

                    send((*kill)->s_client, "", 0, 0);
                    close_cli(tags, (*kill)->s_client);
                    close((*kill)->s_client);
                    *kill = (*kill)->next;
                }
                *s = -1;
                return(data);
            }
            else
                logexit("get_cli_by_tag");
            pthread_mutex_unlock(&mutex);
        
        }
        // Tentativa de cadastrar tags
        if(n_token[0] == '+' || n_token[0] == '-') {
            for(int i=1; i<strlen(n_token); i++){
                if((n_token[i] < 65 || n_token[i] > 90) && (n_token[i] < 97 || n_token[i] > 122)){
                    printf("[log] client %i tried to register / unsubscribe tag with invalid characters\n", *s);
                    // char *erro_tag = "Attempted interest / disinterest with one or more invalid characters. Please try again\n";
                    // size_t count = send(*s, erro_tag,strlen(erro_tag), 0);
                    // if (count != strlen(erro_tag)) 
                    //         logexit("send");
                    tag_flag = 1;
                    break;
                }
            }
            if(tag_flag == 0) {
                char token[500] = {0};
                strcpy(token, n_token+1);
                if(n_token[0] == '+'){
                    pthread_mutex_lock(&mutex);
                    int ans = add_tag(tags, token, *s);
                    pthread_mutex_unlock(&mutex);
                    if(ans == 0){
                        
                        char ok_tag[500] = "subscribed +";
                        strcat(ok_tag, token);
                        strcat(ok_tag, "\n");
                        size_t count = send(*s, ok_tag, strlen(ok_tag), 0);
                        if (count != strlen(ok_tag)) 
                            logexit("send");
                        printf("[log] client %i registered in tag\n", *s);
                    }
                    else if(ans == 1){
                        char alr_tag[500] = "already subscribed +";
                        strcat(alr_tag, token);
                        strcat(alr_tag, "\n");
                        size_t count = send(*s, alr_tag, strlen(alr_tag), 0);
                        if (count != strlen(alr_tag)) 
                            logexit("send");
                        printf("[log] client %i already registered in tag\n", *s);
                    }
                    else
                        logexit("add_cli_in_tag");
                }
                else if(n_token[0] == '-'){
                    pthread_mutex_lock(&mutex);
                    int ans = rem_cli(tags, token, *s);
                    pthread_mutex_unlock(&mutex);
                    if(ans == 0){
                        char ok_tag[500] = "not subscribed -";
                        strcat(ok_tag, token);
                        strcat(ok_tag, "\n");
                        size_t count = send(*s, ok_tag, strlen(ok_tag), 0);
                        if (count != strlen(ok_tag)) 
                            logexit("send");
                        printf("[log] client %i was not registered in tag\n", *s);
                    }
                    else if(ans == 1){
                        char alr_tag[500] = "unsubscribed -";
                        strcat(alr_tag, token);
                        strcat(alr_tag, "\n");
                        size_t count = send(*s, alr_tag, strlen(alr_tag), 0);
                        if (count != strlen(alr_tag)) 
                            logexit("send");
                        printf("[log] client %i now is no longer registered in tag\n", *s);
                    }
                    else
                        logexit("rem_cli_of_tag");
                }
                    
            }

        }
        // Tentativa de mandar msg para outros clientes
        else {
            pthread_mutex_lock(&mutex);
            int_list *copy = criar_lc();
            int_list *to_send = criar_lc();
            pthread_mutex_unlock(&mutex);

            char msg[500] = {0};
            char* end_s;
            s_token = strtok_r(n_token, " ", &end_s); 
            while ( s_token != NULL) {
                int ht_flag = 0;
                if(s_token[0] == '#') {
                    ht_flag = 1;     // se houver outra # -> mensagem / senão -> tag
                    for(int i=1; i<strlen(s_token); i++){
                        if(s_token[i] == '#'){
                            ht_flag = 0;
                            break;
                        }
                    }
                }
      
                if(ht_flag == 1){   //mandar msg
                    char token[500] = {0};
                    sprintf(token, "%s", s_token+1);
                    pthread_mutex_lock(&mutex);
                    copy = get_cli_bt_id(tags, token);
                    pthread_mutex_unlock(&mutex);
                    
                    if(copy != NULL) {
                        while (*copy != NULL) {
                            add_c(to_send, (*copy)->s_client);
                            (*copy) = (*copy)->next;
                        }
                        memset(copy, 0, sizeof(int_list));
                    }
    
                }

                strcat(msg, s_token);
                strcat(msg, " ");
           
                s_token = strtok_r(NULL, " ", &end_s);

            }

            int g = strlen(msg) - 1;
            msg[g] = '\n';


            if(to_send != NULL) {  //mandar msg para outros clientes
                if(*to_send != NULL)
                    printf("[log] client %i send message to other clients\n", *s);
                else
                    printf("[log] client %i did not send message (no tag or no clients in tags)\n", *s);
                while (*to_send != NULL ) {
                    if(((*to_send)->s_client) != *s){
                        size_t count = send(((*to_send)->s_client), msg, strlen(msg), 0);
                        if (count != strlen(msg)) 
                            logexit("send");
                    }
                    
                    (*to_send) = (*to_send)->next;
                }
                
            }

            memset(msg, 0, BUFSZ);

        }
        n_token = strtok_r(NULL, "\n", &end_n);
    }

    
    pthread_exit(EXIT_SUCCESS);
}

int handle_new_connection (void *sok) {
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);
    int *s = (int *)sok;

    int csock = accept(*s, caddr, &caddrlen);
    if (csock == -1)
        logexit("accept");

    struct client_data *cdata = malloc(sizeof(*cdata));
    if (!cdata) 
        logexit("malloc");
    
    cdata->csock = csock;
    memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);
    printf("[log] socket: %i\n", cdata->csock);

    add_tag(tags, "##kill", cdata->csock);

    return(cdata->csock);
}

int main(int argc, char **argv) {

    tags = criar_tl();

    if(argc < 2)
        usage(argc, argv);

    pthread_mutex_init(&mutex, NULL);
 
    struct sockaddr_storage storage;
    if(0 != server_sockaddr_init("v4", argv[1], &storage))
        usage(argc, argv);

    int serverSocket = socket(storage.ss_family, SOCK_STREAM, 0);
    if(serverSocket == -1) 
        logexit("socket");
    
    int enable = 1;
    if (0 != setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
        logexit("setsockopt");

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(0 != bind(serverSocket, addr, sizeof(storage)))
        logexit("bind");

    if(0 != listen(serverSocket, 10))
        logexit("listem");
    
    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting conections\n", addrstr);

    struct client_data *cdata = malloc(sizeof(*cdata));
    if (!cdata) 
        logexit("malloc");

    //para o select
    fd_set current_sockets, ready_sockets;
    FD_ZERO(&current_sockets);
    FD_SET(serverSocket, &current_sockets);
    int max_socket_so_far = serverSocket;
    
    while (1) {
        pthread_t tid;
        void *retVal = NULL;   //para o retorno em caso de fechamento do cliente
        ready_sockets = current_sockets;
        int aux = select(max_socket_so_far+1, &ready_sockets, NULL, NULL, NULL);
        if(aux < 0)
            logexit("select");
        
        for(int i=0; i <= max_socket_so_far; i++) {
            if(FD_ISSET(i, &ready_sockets)) {
                if(i == serverSocket) {
                    int c_sok = handle_new_connection(&serverSocket);   //novo cliente
                    FD_SET(c_sok, &current_sockets);
                    if(c_sok > max_socket_so_far)
                        max_socket_so_far = c_sok + 1;
                }
                else {
                    pthread_create(&tid, NULL, send_client_thread, &i);  //nova mensagem
                    pthread_join(tid, &retVal);
                    if(retVal != NULL){
                        if(*((int *)retVal) == -1) {
                            
                            FD_ZERO(&current_sockets);
                            exit(EXIT_SUCCESS);
                        }
                        else if(*((int *)retVal) == i){
                            FD_CLR(i, &current_sockets);
                            
                        }
                    }
                    
                }    
            }
        }
    }

    exit(EXIT_SUCCESS);
}