#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "drone_movement.h"

#define PIPE_LOCATION "./pipe"
#define MAX_DRONES 4

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

void init_bases(Base *bases);
int init_drones(Drone *drones, Base *bases);
void *manage_drones(void *id_ptr);
int move_to_warehouse(Drone *drone);
void end_drones();
int create_pipe();
int central_proc(int shmid);
