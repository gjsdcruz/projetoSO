#include "central_proc.h"

Warehouse w1;

pthread_t drone_threads[MAX_DRONES];
Shm_Struct *shared_memory;

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
  while(1) {
    printf("DRONE %d AT (%f, %f)\n", drone->id, drone->x, drone->y);
    if(drone->state) {
      move_to_warehouse(drone, &w1);
      shared_memory->products_loaded += 2;
      drone->state = 0;
      shared_memory->products_delivered += 2;
    }
    sleep(5);
  }
}

// Moves designated drone to designated warehouse
// Returns 0 on arrival, 1 on failure
int move_to_warehouse(Drone *drone, Warehouse *w1) {
  int res;
  do {
    res = move_towards(&(drone->x), &(drone->y), w1->x, w1->y);
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
void create_pipe() {
  if(mkfifo(PIPE_LOCATION, O_CREAT|O_EXCL|0600) < 0) {
    perror("CAN'T CREATE PIPE: ");
    exit(0);
  }
}

// Process running the Central
int central_proc(Shm_Struct *shm) {

  // Enables use of shared memory by Central and Drones
  shared_memory = shm;

  //###################SIGNAL HANDLING###############################

  // Central must ignore SIGUSR1
  sigset_t block;
  sigemptyset(&block);
  sigaddset(&block, SIGUSR1);
  sigprocmask(SIG_BLOCK, &block, NULL);

  //###################################################################

  Base bases[4];
  Drone drones[MAX_DRONES];
  w1.x = 400.0;
  w1.y = 100.0;
  init_bases(bases);
  if(init_drones(drones, bases)) {
    return 1;
  }
  drones[2].state = 1;
  shared_memory->orders_given++;
  end_drones();
  create_pipe();
  unlink(PIPE_LOCATION);
  return 0;
}
