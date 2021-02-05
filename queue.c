#include "queue.h"

int_list *criar_lc()
{
    int_list *lc = (int_list *)malloc(sizeof(int_list));
    if (lc != NULL)
        return lc;
    *lc = NULL;
    return lc;
}

int add_c(int_list *lc, int s) {
    if (lc == NULL)
        return(-1);

    struct no *elem = malloc(sizeof(*elem));
    if (elem == NULL)
        return(-1);

    elem->s_client = s;
    elem->next = NULL;

    int equal_flag = 0;

    if ((*lc) == NULL)
        *lc = elem;
    else {
        struct no *aux = *lc;
        while (aux->next != NULL) {
            if(aux->s_client == s){
                equal_flag = 1;
                break;
            }
            aux = aux->next;
        }
        if(aux->s_client == s)
            equal_flag = 1;
        
        if(equal_flag == 0)
            aux->next = elem;
    }
    return(equal_flag);
}

int rem_c(int_list *lc, int s, int flag) {
    if (lc == NULL)
        return(-1);

    if ((*lc) == NULL)
        return(-1);
    
    
    int equal_flag = 0;
    struct no *prox = (*lc)->next, *ant = *lc;


    if(ant->s_client == s){
        equal_flag = 1;
        if(ant->next == NULL)
            *lc = NULL;
        else
            *ant = *prox;  
    }
    else while (prox != NULL) {
        if(prox->s_client == s){
            equal_flag = 1;
            if(prox->next == NULL)
                ant->next = NULL;
            else {
                ant->next = prox->next;
                free(prox);
            }
            break;
        }
        ant = ant->next;
        prox = prox->next;
    }
    return(equal_flag);
}

tag_list *criar_tl()
{
    tag_list *tl = (tag_list *)malloc(sizeof(tag_list));
    if (tl != NULL)
        *tl = NULL;

    struct tag_no *elem = malloc(sizeof(struct tag_no));
    if (elem == NULL)
        return tl;
    elem->c_socks = criar_lc();
    memcpy(elem->msg, "##kill", 6);
    elem->next = NULL;
    
    *tl = elem;
    
    return tl;
}

int add_tag(tag_list *tl, char *buf, int csock) {
    if (tl == NULL)
        return(-1);

    add_c((*tl)->c_socks, csock);
    
    int cli_flag = 0, tag_flag = 0;
    struct tag_no *aux = *tl;
    while (aux->next != NULL) {
        if(memcmp(aux->msg, buf, strlen(buf)) == 0){
            cli_flag = add_c(aux->c_socks, csock);
            tag_flag = 1;
            break;
        }
        aux = aux->next;
    }

    if(memcmp(aux->msg, buf, strlen(buf)) == 0 && tag_flag == 0){
        cli_flag = add_c(aux->c_socks, csock);
        tag_flag = 1;
    }
    if(tag_flag == 0){
        struct tag_no *elem = malloc(sizeof(*elem));
        if (elem == NULL)
            return(-1);
        
        elem->c_socks = criar_lc();
        add_c(elem->c_socks, csock);
        memcpy(elem->msg, buf, strlen(buf));
        elem->next = NULL;

        aux->next = elem;
    }
    
    return(cli_flag);
        
}

int rem_cli(tag_list *tl, char *buf, int csock) {
    if (tl == NULL)
        return(-1);

    int cli_flag = 0, tag_flag = 0;
    struct tag_no *ant = *tl, *prox = (*tl)->next;
    
    if(memcmp(ant->msg, buf, strlen(buf)) == 0){
        tag_flag = 1;
        cli_flag  = rem_c(ant->c_socks, csock, 1);
        
        return(cli_flag);
    }

    while(prox != NULL) {
        if(memcmp(prox->msg, buf, strlen(buf)) == 0){
            tag_flag = 1;
            cli_flag  = rem_c(prox->c_socks, csock, 0);
            break;
        }
        ant = ant->next;
        prox = prox->next;
    }
    
    if(tag_flag == 0)
        return(tag_flag);
    else{
        if((*(prox->c_socks)) == NULL)
            ant->next = prox->next;
        
        return(cli_flag);

    }
}

void close_cli(tag_list *tl, int csock) {
    if (tl == NULL || (*tl) == NULL)
        return;

    struct tag_no *aux = (*tl)->next;
    rem_cli(tl, (*tl)->msg, csock);
    
    while (aux != NULL) {
        rem_cli(tl, aux->msg, csock);
        aux = aux->next;
    }

}

int_list *get_cli_bt_id(tag_list *tl, char *buf) {
    
    if (tl == NULL || (*tl) == NULL)
        return(NULL);

    int_list *lc = (int_list *)malloc(sizeof(int_list));
    int flag = 0;
    struct tag_no *aux = *tl;

    while (aux != NULL && aux->next != NULL) {
        if(memcmp(aux->msg, buf, strlen(buf)) == 0){
            struct no *x = *(aux)->c_socks;
            while (x != NULL){
                flag = 1;
                add_c(lc, x->s_client);
                x = x->next;
            }            
            flag = 1;
            break;
        }
        aux = aux->next;
    }
    if(memcmp(aux->msg, buf, strlen(buf)) == 0){
        struct no *x = *(aux)->c_socks;
        if(x != NULL){
            while (x->next != NULL){
                flag = 1;
                add_c(lc, x->s_client);
                x = x->next;
            }            
            flag = 1;
            add_c(lc, x->s_client);
        }
    }

    if(flag == 1)
        return(lc);
    else{
        return(NULL);
    }
}

void print_tl(tag_list *tl) {
    if (tl == NULL || (*tl) == NULL)
        return;

    struct tag_no *aux = *tl;

    while (aux != NULL) {
        struct no *x = *(aux)->c_socks;
        printf("tag: %s\n", aux->msg);
        printf("lista: ");
        while (x != NULL){
            printf("%i ", x->s_client);
            x = x->next;
        }
        printf("\n");
        aux = aux->next;
    }
}