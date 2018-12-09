/******************************
Code created by:
   Diogo Ferrer, 2017247199
   Guilherme Cruz, 2016223968
*******************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/msg.h>
#include "drone_movement.h"
#include "sim_manager.h"

#define PIPE_LOCATION "./pipe"
#define MAX_CMD_SIZE 200
#define WORD_SIZE 50

typedef struct {
  int id;
  double x;
  double y;
  int state; // 0 - Unoccupied, 1 - Occupied
  struct order *curr_order; // NULL if none assigned
} Drone;

typedef struct {
  int id;
  double x;
  double y;
} Base;

typedef struct order{
  int id;
  char *name, *prod;
  wnode_t *wh;
  int quantity;
  int x, y;
  long start_time, end_time;
} order_t;

typedef struct onode {
  order_t *order;
  struct onode *next;
} onode_t;

void init_bases(int max_x, int max_y);
int init_drones();
void *manage_drones(void *id_ptr);
void move_to_base(Drone *drone);
void move_to_destination(Drone *drone);
void move_to_warehouse(Drone *drone, wnode_t *w);
void end_drones();
void *handle_orders_list();
void *handle_drone_changes();
void create_aux();
void end_aux();
void destroy_mutexes();
void create_pipe();
void add_order_node(order_t *new_order);
void read_cmd(pnode_t *phead, int max_x, int max_y);
void handle_order(order_t *order);
void change_drones(int num_drones);
int central_proc(int max_x, int max_y, int n_of_drones, Shm_Struct *shared_memory, pnode_t *phead);
