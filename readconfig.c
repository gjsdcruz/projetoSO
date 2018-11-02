#include <stdio.h>

typedef struct node {
    int val;
    struct node* next;
} node_t;

typedef struct llist {
    node_t* product_head;
    node_t* warehouse_head;
} llist_t;

llist_t* readconfig(int* max_x, int* max_y){
    llist_t info;
    info.product_head = NULL;
    info.warehouse_head = NULL;

    FILE* conf;
    char* buffer[255];
    conf = fopen("config.txt", "r");
    fscanf(conf, "%e, %e", max_x, max_y);
    while(fgetc(conf) != "\n"){
        fscanf(conf, "%s", buffer);
        
    }
}