#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include "sim_manager.h"
#include "central_proc.h"
#include "log.h"

// Product node struct
typedef struct pnode {
    char* name;
    struct pnode* next;
} pnode_t;

// Warehouse product node struct
typedef struct wpnode {
    char* name;
    int quantity;
    struct wpnode* next;
} wpnode_t;

// Warehouse node struct
typedef struct wnode {
    char* name;
    int chartx, charty;
    wpnode_t* plist_head;
    struct wnode* next;
} wnode_t;

// Config list struct
typedef struct llist {
    int max_x;
    int max_y;
    pnode_t* product_head;
    wnode_t* warehouse_head;
    int* n_of_drones, refill_rate, quantity, time_unit, *n_of_whouses;
} llist_t;

int main(int argc, char const *argv[]){

  log_it("SIMULATION INITIATED");

//################# CONFIG PARSING ################################
// Chunk of code parses info from config.txt to data struct "info" of type llist_t
    llist_t* info;
    info->product_head = NULL;
    info->warehouse_head = NULL;
    info->warehouse_head->plist_head = NULL;

    
    FILE* conf;
    char buffer[255];
    const char delim[2] = ", ";
    char* token;
    conf = fopen("config.txt", "r");
    fscanf(conf, "%s ", buffer);
    token = strtok(buffer, delim);
    info->max_x = atoi(token);
    token = strtok(NULL, delim);
    info->max_y = atoi(token);

    fscanf(conf, "%s ", buffer);
    const char delim2[2] = " ";
    token = strtok(buffer, delim2);
    while(token != NULL){
        if(info->product_head == NULL){
            pnode_t* phead;
            phead->name = token;
            phead->next = NULL;

            info->product_head = phead;
        } else {
            pnode_t* aux = (pnode_t*) malloc (sizeof(pnode_t));
            pnode_t* nav = info->product_head;
            aux->name = token;
            aux->next = NULL;
            while(nav->next != NULL){
                nav = nav->next;
            }
            nav->next = aux;
            free(aux);
        }
    fscanf(conf, "%i", info->n_of_drones);

    fscanf(conf, "%s ", buffer);
    token = strtok(buffer, delim);
    info->quantity = atoi(token);
    token = strtok(NULL, delim);
    info->refill_rate = atoi(token);
    token = strtok(NULL, delim);
    info->time_unit = atoi(token);
    
    fscanf(conf, "%i", info->n_of_whouses);

    
    int i;
    for (i=1; i<*(info->n_of_whouses); i++){
        fscanf(conf, "%s ", buffer);
        token = strtok(buffer, delim2);
        if(info->warehouse_head == NULL){
            wnode_t* whead;
            whead->name = token;
            token = strtok(NULL, delim2);
            token = strtok(NULL, delim);
            whead->chartx = atoi(token);
            token = strtok(NULL, delim2);
            whead->charty = atoi(token);
            token = strtok(NULL, delim2);
            while(token != NULL){
                if (whead->plist_head == NULL){
                    wpnode_t* head;
                    token = strtok(NULL, delim);
                    head->name = token;
                    token = strtok(NULL, delim);
                    head->quantity = atoi(token);
                    head->next = NULL;
                    whead->plist_head = head;
                    free(head);
                }
            }
            whead->next = NULL;
            info->warehouse_head = whead;
        } else {
            wnode_t* aux = (wnode_t*) malloc (sizeof(wnode_t));
            wnode_t* nav = info->warehouse_head;
            aux->name = token;
            token = strtok(NULL, delim2);
            token = strtok(NULL, delim);
            aux->chartx = atoi(token);
            token = strtok(NULL, delim2);
            aux->charty = atoi(token);
            token = strtok(NULL, delim2);
            while(token != NULL){
                if (aux->plist_head == NULL){
                    wpnode_t* head;
                    token = strtok(NULL, delim);
                    head->name = token;
                    token = strtok(NULL, delim);
                    head->quantity = atoi(token);
                    head->next = NULL;
                    aux->plist_head = head;
                    free(head);
                }
            }
            while(nav->next != NULL){
                nav = nav->next;
            }
            nav->next = aux;
            free(aux);
        }
    }

    }

//####################################################################


//###################SHARED MEMORY CREATION###########################


    int shared_ints[4], shared_doubles[2];
    int shmid, i;
    sem_t *mutex;

	shmid = shmget(IPC_PRIVATE, __getpagesize(),IPC_CREAT|0700);
    i=0;
    for(i=0; i<4; i++){
	    shared_ints[i] = (int*) shmat(shmid, NULL, 0);
    }
    for(i=0; i<2; i++){
        shared_doubles[i] = (double*) shmat(shmid, NULL, 0);
    }


//####################################################################

//###################PROCESS MANAGEMENT###############################

    pid_t process = fork();
    if (process == 0){
        central_proc(shmid);
        exit(0);
    } else {
      wait(NULL);
    }
    shmctl(shmid, IPC_RMID, NULL);

//###################################################################

    log_it("SIMULATION TERMINATED");

    return 0;
}
