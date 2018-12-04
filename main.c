#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include "sim_manager.h"
#include "central_proc.h"
#include "log.h"

int main(int argc, char const *argv[]){

  // Config variables
  int max_x;
  int max_y;
  pnode_t* product_head;
  int n_of_drones, refill_rate, quantity, n_of_whouses;

//################# CONFIG PARSING ################################
// Chunk of code parses info from config.txt to data struct "info" of type llist_t

    char header[5][256];
    char *token;

    const char delim[3] = ", ";

    product_head = NULL;

    FILE *conf = fopen("config.txt", "r");

    if (conf == NULL){
        perror("Config file not found");
        return 0;
    }

    for (int i=0; i<5; i++){
        fgets(header[i], 255, conf);
    }

    token = strtok(header[0], delim);
    max_x = atoi(token);
    token = strtok(NULL, delim);
    max_y = atoi(token);

    token = strtok(header[1], delim);
    int j = 0;
    while(token != NULL){
        if(product_head == NULL){
            product_head = (pnode_t*) malloc(sizeof(pnode_t));
            product_head->name = (char*) malloc(sizeof(char)*30);
            product_head->next = NULL;
            strcpy(product_head->name, token);
        } else {
            pnode_t* nav = product_head;
            pnode_t* aux = (pnode_t*) malloc(sizeof(pnode_t));
            aux->name = (char*) malloc(sizeof(char)*30);
            strcpy(aux->name, token);
            aux->next = NULL;

            while (nav->next != NULL){
                nav = nav->next;
            }
            nav->next = aux;
        }
        token = strtok(NULL, delim);
        j++;
    }

    n_of_drones = atoi(header[2]);
    token = strtok(header[3], delim);
    refill_rate = atoi(token);
    token = strtok(NULL, delim);
    quantity = atoi(token);
    token = strtok(NULL, delim);
    time_unit = atof(token);

    token = strtok(header[4], delim);
    n_of_whouses = atoi(token);

    char whouse_data [n_of_whouses][255];
    wnode_t whouse[n_of_whouses];

    for(int i = 0; i<n_of_whouses; i++){
        fgets(whouse_data[i], 255, conf);
    }

    for(int i = 0; i<n_of_whouses; i++){
        whouse[i].id = i+1;
        token = strtok(whouse_data[i], " ");
        whouse[i].name = token;
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");
        whouse[i].chartx = atoi(token);
        token = strtok(NULL, delim);
        whouse[i].charty = atoi(token);
        whouse[i].plist_head = NULL;
        token = strtok(NULL, " ");
        token = strtok(NULL, delim);
        while(token != NULL){
            if(whouse[i].plist_head == NULL){
                whouse[i].plist_head = (wpnode_t*) malloc(sizeof(wpnode_t));
                (whouse[i].plist_head)->name = token;
                token = strtok(NULL, delim);
                if(token == NULL){
                    perror("Config file error: Warehouse product description mismatch\n");
                    return 0;
                }
                (whouse[i].plist_head)->quantity = atoi(token);
                (whouse[i].plist_head)->next = NULL;
            } else {
                wpnode_t* navg = whouse[i].plist_head;
                wpnode_t* auxw = (wpnode_t*) malloc (sizeof(wpnode_t));
                auxw->name = token;
                token = strtok(NULL, delim);
                if(token == NULL){
                    perror("Config file error: Warehouse product description mismatch\n");
                    return 0;
                }
                auxw->quantity = atoi(token);
                auxw->next = NULL;

                while(navg->next != NULL){
                    navg = navg->next;
                }
                navg->next = auxw;
            }
            token = strtok(NULL, delim);
        }
    }

//####################################################################

  sim_manager(max_x, max_y, product_head, n_of_drones, refill_rate, quantity, n_of_whouses, whouse);
  return 0;
}
