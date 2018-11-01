#include "central_proc.h"

pthread_t drone_threads[MAX_DRONES];

// Creates the bases
void init_bases(Base *bases) {
  double max_x = 1200.0, max_y = 800.0;
  for(int i = 0; i < 4; i++) {
    bases[i].id = i;
    if(i < 2)
      bases[i].y = 0.0;
    else
      bases[i].y = max_y;
    if(i % 3)
      bases[i].x = max_x;
    else
      bases[i].x = 0.0;
  }
}

// Creates the drone threads.
// Returns 0 on success, 1 on failure.
int init_drones(Drone *drones, Base *bases) {
  for(int i = 0; i < MAX_DRONES; i++) {
    drones[i].id = i;
    drones[i].state = 0;
    drones[i].x = bases[i % 4].x;
    drones[i].y = bases[i % 4].y;
    pthread_create(&drone_threads[i], NULL, manage_drones, &drones[i]);
    printf("DRONE %d CREATED\n", drones[i].id);
  }
  return 0;
}

// Manages drone behaviour for each event
void *manage_drones(void *drone_ptr) {
  Drone *drone = (Drone*) drone_ptr;
  move_to_warehouse(drone);
  printf("DRONE %d IS AT LOCATION (%f, %f)\n", drone->id, drone->x, drone->y);
}

// Moves designated drone to designated warehouse
// Returns 0 on arrival, 1 on failure
int move_to_warehouse(Drone *drone) {
  int res;
  double x = 400.0, y = 100.0;
  do {
    res = move_towards(&(drone->x), &(drone->y), x, y);
    if(res == -2)
      return 1;
  } while(res != -1);
  return 0;
}

// Waits for all the drone threads to join
void end_drones() {
  for(int i = 0; i < MAX_DRONES; i++) {
    pthread_join(drone_threads[i], NULL);
    printf("DRONE %d JOINED\n", i);
  }
}

// Creates FIFO pipe.
// Returns 0 on success and 1 on failure.
int create_pipe() {
  if(mkfifo(PIPE_LOCATION, O_CREAT|O_EXCL|0600) < 0) {
    perror("Can't create pipe: ");
    return 1;
  }
  return 0;
}

// Process running the Central
int central_proc(int shmid) {
  Base bases[4];
  Drone drones[MAX_DRONES];
  init_bases(bases);
  if(init_drones(drones, bases)) {
    return 1;
  }
  end_drones();
  create_pipe();
  unlink(PIPE_LOCATION);
  return 0;
}
