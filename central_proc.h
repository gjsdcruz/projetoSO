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
#include "drone_movement.h"
#include "sim_manager.h"

#define PIPE_LOCATION "./pipe"
#define MAX_CMD_SIZE 100
#define WORD_SIZE 10

typedef struct {
  double x;
  double y;
} Warehouse;

typedef struct {
  int id;
  double x;
  double y;
  int state; // 0 - Unoccupied, 1 - Occupied
} Drone;

typedef struct {
  int id;
  double x;
  double y;
} Base;

typedef struct {
  char *name, *prod;
  int quantity;
  int x, y;
} order_t;

void init_bases(Base *bases, int max_x, int max_y);
int init_drones(Drone *drones, Base *bases);
void *manage_drones(void *id_ptr);
int move_to_warehouse(Drone *drone, wnode_t *w);
void end_drones();
void create_pipe();
void read_cmd(pnode_t *phead, int max_x, int max_y);
void handle_order(order_t order);
void change_drones(int num_drones);
int central_proc(int max_x, int max_y, int n_of_drones, Shm_Struct *shared_memory, pnode_t *phead);
