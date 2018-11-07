#include "central_proc.h"

pthread_t *drone_threads;
Shm_Struct *shared_memory;
int n_drones;

// Creates the bases
void init_bases(Base *bases, int max_x, int max_y) {
  for(int i = 0; i < 4; i++) {
    bases[i].id = i;
    if(i < 2)
      bases[i].y = 0.0;
    else
      bases[i].y = (double)max_y;
    if(i % 3)
      bases[i].x = (double)max_x;
    else
      bases[i].x = 0.0;
  }
}

// Creates the drone threads.
// Returns 0 on success, 1 on failure.
int init_drones(Drone *drones, Base *bases) {
  drone_threads = (pthread_t*) malloc(sizeof(pthread_t));
  for(int i = 0; i < n_drones; i++) {
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
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      move_to_warehouse(drone, &(shared_memory->warehouses[0]));
      shared_memory->products_loaded += 2;
      drone->state = 0;
      shared_memory->products_delivered += 2;
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    sleep(2);
  }
}

// Moves designated drone to designated warehouse
// Returns 0 on arrival, 1 on failure
int move_to_warehouse(Drone *drone, wnode_t *w) {
  int res;
  do {
    res = move_towards(&(drone->x), &(drone->y), w->chartx, w->charty);
    if(res == -2)
      return 1;
  } while(res != -1);
  return 0;
}

// Waits for all the drone threads to join
void end_drones() {
  for(int i = 0; i < n_drones; i++) {
    pthread_cancel(drone_threads[i]);
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

void end_signal_handler(int signum) {
  end_drones();
  unlink(PIPE_LOCATION);
  exit(0);
}

// Process running the Central
int central_proc(int max_x, int max_y, int n_of_drones, Shm_Struct *shm) {

  // Enables use of shared memory by Central and Drones
  shared_memory = shm;

  n_drones = n_of_drones;

  //###################SIGNAL HANDLING###############################

  // Central must ignore SIGUSR1 and SIGINT
  sigset_t block;
  sigemptyset(&block);
  sigaddset(&block, SIGUSR1);
  sigaddset(&block, SIGINT);
  sigprocmask(SIG_BLOCK, &block, NULL);

  // Central must handle SIGUSR2
  struct sigaction end_action;
  end_action.sa_handler = end_signal_handler;
  sigemptyset(&end_action.sa_mask);
  end_action.sa_flags = 0;
  sigaction(SIGUSR2, &end_action, NULL);

  //###################################################################

  Base bases[4];
  Drone drones[n_drones];

  init_bases(bases, max_x, max_y);
  if(init_drones(drones, bases)) {
    return 1;
  }
  drones[0].state = 1;
  shared_memory->orders_given++;
  create_pipe();
  while(1) {

  }
  return 0;
}
