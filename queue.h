#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h> 

// struct client_data {
//     int csock;
//     sockaddr_storage storage;
// };

// struct node {
//     node* next;
//     client_data data;
// };

struct no
{
    int s_client;
    struct no *next;
};

typedef struct no *int_list;

struct tag_no
{
    char msg[500];
    int_list* c_socks;
    struct tag_no *next;    
};

typedef struct tag_no *tag_list;

int_list *criar_lc();

int add_c(int_list *lc, int s);   //se ja tinha = 1 / senao = 0
int rem_c(int_list *lc, int s, int flag);   //se ja tirou = 1 / senao = 0

tag_list *criar_tl();

int add_tag(tag_list *tl, char *buf, int csock);   //adciona cliente de uma tag

int rem_cli(tag_list *tl, char *buf, int csock);   //remove cliente de uma tag

void close_cli(tag_list *tl, int csock);           //remove cliente de todas as tags

void kill(tag_list *tl);                           //remove tudo

int_list *get_cli_bt_id(tag_list *tl, char *buf);  //retorna lista de clientes de uma tag

void print_tl(tag_list *tl);                       //printa a lista
 
#endif