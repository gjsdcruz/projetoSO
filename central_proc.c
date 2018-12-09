#include "central_proc.h"
#include "log.h"

#define DEBUG
#define MOVEMENT_DEBUG

pthread_t **drone_threads;
pthread_cond_t state_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t **state_mutexes;
Shm_Struct *shared_memory;
int n_drones, order_id = 100, drone_id = 1;
Base bases[4];
Drone **drones;
onode_t *orders_list = NULL;
int fd;

// Creates the bases
void init_bases(int max_x, int max_y) {
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

// Creates the drone threads and respective mutexes.
// Returns 0 on success, -1 on failure.
int init_drones() {
  drone_threads = (pthread_t**) malloc(n_drones * sizeof(pthread_t*));
  state_mutexes = (pthread_mutex_t**) malloc(n_drones * sizeof(pthread_mutex_t*));
  if(!drone_threads || !state_mutexes) {
    return -1;
  }
  for(int i = 0; i < n_drones; i++) {
    state_mutexes[i] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    *state_mutexes[i] = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    drone_threads[i] = (pthread_t*) malloc(sizeof(pthread_t));
    drones[i] = (Drone*) malloc(sizeof(Drone));
    drones[i]->id = drone_id++;
    drones[i]->state = 0;
    drones[i]->curr_order = NULL;
    drones[i]->x = bases[i % 4].x;
    drones[i]->y = bases[i % 4].y;
    pthread_create(drone_threads[i], NULL, manage_drones, drones[i]);
    #ifdef DEBUG
    printf("DRONE %d CREATED\n", drones[i]->id);
    #endif
  }
  return 0;
}

// Manages drone behaviour for each event
void *manage_drones(void *drone_ptr) {
  Drone *drone = (Drone*) drone_ptr;
  while(1) {
    if(drone->state) {
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      move_to_warehouse(drone, drone->curr_order->wh);

      // Sends message to MQ to notify WH of arrival
      msg_to_wh msg;
      msg.wh_id = (long)drone->curr_order->wh->id;
      msg.msgtype = DRONE_ARRIVAL_TYPE;
      msg.order_id = drone->curr_order->id;
      msg.quantity = drone->curr_order->quantity;
      msgsnd(mq_id, &msg, sizeof(msg_to_wh), 0);

      // Wait for warehouse to send message back
      msg_from_wh in_msg;
      long msg_type = (long)drone->curr_order->id;

      msgrcv(mq_id, &in_msg, 0, msg_type, 0);

      move_to_destination(drone);
      drone->curr_order->end_time = clock();
      char log_msg[MSG_SIZE];
      sprintf(log_msg, "ORDER %s-%d DELIVERED", drone->curr_order->name, drone->curr_order->id);
      log_it(log_msg);

      // Update statistics
      double order_time = (double)(drone->curr_order->end_time - drone->curr_order->start_time) / CLOCKS_PER_SEC * 1000;
      sem_wait(shm_sem);
      shared_memory->avg_time = (double)(shared_memory->avg_time*shared_memory->orders_delivered + order_time) / (shared_memory->orders_delivered + 1);
      shared_memory->orders_delivered++;
      shared_memory->products_delivered += drone->curr_order->quantity;
      sem_post(shm_sem);

      // Release drone and send it to closest base
      drone->curr_order = NULL;
      drone->state = 0;
      move_to_base(drone);
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    else {
      pthread_mutex_lock(state_mutexes[drone->id-1]);
      pthread_cond_wait(&state_cond, state_mutexes[drone->id-1]);
      pthread_mutex_unlock(state_mutexes[drone->id-1]);
    }
  }
}

// Moves designated drone to order destination
void move_to_destination(Drone *drone) {
  int res;
  do {
    #ifdef MOVEMENT_DEBUG
    printf("DRONE %d LOCATION: (%lf, %lf) GOING TO (%lf, %lf)\n", drone->id, drone->x, drone->y, (double)drone->curr_order->x, (double)drone->curr_order->y);
    #endif
    res = move_towards(&(drone->x), &(drone->y), (double)drone->curr_order->x, (double)drone->curr_order->y);
    if(res == -2)
      return;
    usleep(time_unit * 1000000);
  } while(res != -1);
  usleep(time_unit * 1000000);
}

// Moves designated drone to closest base
void move_to_base(Drone *drone) {
  // Find closest base
  double min_distance = 0, dist;
  Base *chosen_base = NULL;
  for(int i = 0; i < 4; i++) {
    if((dist = distance(drone->x, drone->y, bases[i].x, bases[i].y)) < min_distance || !min_distance) {
      min_distance = dist;
      chosen_base = &bases[i];
    }
  }

  // If base was found, move drone but check if there is new order while on route
  if(chosen_base) {
    int res;
    do {
      #ifdef MOVEMENT_DEBUG
      printf("DRONE %d LOCATION: (%lf, %lf) GOING TO (%lf, %lf)\n", drone->id, drone->x, drone->y, chosen_base->x, chosen_base->y);
      #endif
      if(!drone->state) {
        res = move_towards(&(drone->x), &(drone->y), chosen_base->x, chosen_base->y);
        if(res == -2)
          return;
        usleep(time_unit * 1000000);
      }
      else return;
    } while(res != -1);
    #ifdef DEBUG
    printf("DRONE %d ARRIVED AT BASE\n", drone->id);
    #endif
  }
  else return;
}

// Moves designated drone to designated warehouse
// Returns 0 on arrival, 1 on failure
void move_to_warehouse(Drone *drone, wnode_t *w) {
  int res;
  do {
    #ifdef MOVEMENT_DEBUG
    printf("DRONE %d LOCATION: (%lf, %lf) GOING TO (%lf, %lf)\n", drone->id, drone->x, drone->y, w->chartx, w->charty);
    #endif
    res = move_towards(&(drone->x), &(drone->y), w->chartx, w->charty);
    if(res == -2)
      return;
    usleep(time_unit * 1000000);
  } while(res != -1);
  #ifdef DEBUG
  printf("DRONE %d ARRIVED AT %s\n", drone->id, w->name);
  #endif
}

// Waits for all the drone threads to join
void end_drones() {
  for(int i = 0; i < n_drones; i++) {
    pthread_cancel(*drone_threads[i]);
    pthread_join(*drone_threads[i], NULL);
    #ifdef DEBUG
    printf("DRONE %d JOINED\n", drones[i]->id);
    #endif
  }
}

// Creates FIFO pipe.
void create_pipe() {
  if(mkfifo(PIPE_LOCATION, O_CREAT|O_EXCL|0600) < 0) {
    perror("CAN'T CREATE PIPE: ");
    exit(0);
  }
}

// Adds order to orders list
void add_order_node(order_t *new_order) {
  onode_t *new_node = (onode_t*) malloc(sizeof(onode_t));
  new_node->order = new_order;
  new_node->next = NULL;

  onode_t *curr = orders_list;
  if(!curr) {
    orders_list = new_node;
  }
  else {
    while(curr->next) {
      curr = curr->next;
    }
    curr->next = new_node;
  }
}

// Reads and validates command from pipe
void read_cmd(pnode_t *phead, int max_x, int max_y) {
  if((fd = open(PIPE_LOCATION, O_RDONLY)) < 0) {
    perror("CANNOT OPEN PIPE FOR READING: ");
    exit(0);
  }

  char cmd[MAX_CMD_SIZE];
  int n_bytes;
  if((n_bytes = read(fd, cmd, MAX_CMD_SIZE)) > 0) {
    cmd[n_bytes-1] = '\0';
  }
  close(fd);

  char cmd_type[WORD_SIZE];
  sscanf(cmd, "%s", cmd_type);

  if(strcmp(cmd_type, "ORDER") == 0) {
    order_t *order = (order_t*) malloc(sizeof(order_t));
    order->name = (char*) malloc(WORD_SIZE);
    order->prod = (char*) malloc(WORD_SIZE);
    sscanf(cmd, "ORDER %s prod: %[^,] , %d to: %d, %d ", order->name, order->prod, &(order->quantity), &(order->x), &(order->y));

    int found = 0;
    while(phead != NULL && !found) {
      if(strcmp(phead->name, order->prod) == 0) {
        found = 1;
      }
      phead = phead->next;
    }
    if(!found) {
      char msg[MAX_CMD_SIZE];
      sprintf(msg, "%s HAS PRODUCT UNKNOWN: %s", order->name, order->prod);
      log_it(msg);
      return;
    }

    if(order->x > max_x || order->x < 0 || order->y > max_y || order->y < 0) {
      char msg[MAX_CMD_SIZE];
      sprintf(msg, "LOCATION OUT OF BOUNDS: (%d, %d)", order->x, order->y);
      log_it(msg);
      return;
    }

    order->id = order_id++;
    order->start_time = clock();
    char log_msg[MSG_SIZE];
    sprintf(log_msg, "ORDER %s-%d RECEIVED", order->name, order->id);
    log_it(log_msg);

    handle_order(order);
  }

  else if(strcmp(cmd_type, "DRONE") == 0) {
    int num_drones;
    // DOESNT DETECT BUG
    if(sscanf(cmd, "DRONE SET%*c%d", &num_drones) != 1) {
      char msg[MAX_CMD_SIZE];
      sprintf(msg, "COMMAND UNKNOWN: %s", cmd);
      log_it(msg);
      return;
    }

    change_drones(num_drones);
  }

  else {
    char msg[MAX_CMD_SIZE];
    sprintf(msg, "COMMAND UNKNOWN: %s", cmd);
    log_it(msg);
  }
}

// Handles given order
void handle_order(order_t *order) {
  Drone *chosen_drone = NULL;
  wnode_t *chosen_whs[shared_memory->n_wh];
  int k = 0;
  double min_distance = 0;

  // Find available warehouses
  for(int i = 0; i < shared_memory->n_wh; i++) {
    wnode_t *this_wh = &(shared_memory->warehouses[i]);
    wpnode_t *curr = this_wh->plist_head;
    while(curr != NULL) {
      sem_wait(shm_sem);
      if(strcmp(curr->name, order->prod) == 0 && curr->quantity >= order->quantity) {
        chosen_whs[k++] = this_wh;
      }
      sem_post(shm_sem);
      curr = curr->next;
    }
  }

  // Choose closest drone
  for(int i = 0; i < n_drones; i++) {
    if(drones[i]->state == 0) {
      for(int j = 0; j < k; j++) {
        double rtl = distance(drones[i]->x, drones[i]->y, chosen_whs[j]->chartx, chosen_whs[j]->charty);
        double rtd = distance(chosen_whs[j]->chartx, chosen_whs[j]->charty, order->x, order->y);
        double total_dist = rtl + rtd;
        if(total_dist < min_distance || !min_distance) {
          min_distance = total_dist;
          chosen_drone = drones[i];
          order->wh = chosen_whs[j];
        }
      }
    }
  }

  // Checks if warehouse was selected
  if(k) {
    // Reserve stock in shared memory
    wpnode_t *curr = order->wh->plist_head;
    while(strcmp(curr->name, order->prod)) {
      curr = curr->next;
    }
    sem_wait(shm_sem);
    curr->quantity -= order->quantity;
    sem_post(shm_sem);
  }
  else {
    add_order_node(order);

    char log_msg[MSG_SIZE];
    sprintf(log_msg, "%s-%d SUSPENDED DUE TO LACK OF STOCK", order->name, order->id);
    log_it(log_msg);
    return;
  }

  // Checks if drone was selected
  if(chosen_drone) {
    // Notify drone to handle delivery
    chosen_drone->curr_order = order;
    chosen_drone->state = 1;
    sem_wait(shm_sem);
    shared_memory->orders_given++;
    sem_post(shm_sem);

    pthread_cond_broadcast(&state_cond);

    char log_msg[MSG_SIZE];
    sprintf(log_msg, "ORDER %s-%d GIVEN TO DRONE %d", order->name, order->id, chosen_drone->id);
    log_it(log_msg);
  }
  else {
    add_order_node(order);
  }
}

// Changes number of drones
void change_drones(int num_drones) {
  // Do nothing if given invalid number of drones
  if(num_drones == n_drones || num_drones < 0) return;

  // Add new drones
  if(num_drones > n_drones) {
    // New arrays
    Drone **new_drones = (Drone**) malloc(num_drones*sizeof(Drone*));
    pthread_t **new_drone_threads = (pthread_t**) malloc(num_drones*sizeof(pthread_t*));
    pthread_mutex_t **new_state_mutexes = (pthread_mutex_t**) malloc((num_drones-n_drones+drone_id-1)*sizeof(pthread_mutex_t));

    // Put existent drones and mutexes in new arrays
    for(int i = 0; i < n_drones; i++) {
      new_drones[i] = drones[i];
      new_drone_threads[i] = drone_threads[i];
    }
    int aux = drone_id-1;
    for(int i = 0; i < aux; i++) {
      new_state_mutexes[i] = state_mutexes[i];
    }

    // Fill new arrays with new drones and new mutexes
    for(int i = n_drones; i < num_drones; i++) {
      new_state_mutexes[aux] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
      *new_state_mutexes[aux++] = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
      state_mutexes = new_state_mutexes;
      new_drones[i] = (Drone*) malloc(sizeof(Drone));
      new_drone_threads[i] = (pthread_t*) malloc(sizeof(pthread_t));
      new_drones[i]->id = drone_id++;
      new_drones[i]->state = 0;
      new_drones[i]->curr_order = NULL;
      new_drones[i]->x = bases[i % 4].x;
      new_drones[i]->y = bases[i % 4].y;
      pthread_create(new_drone_threads[i], NULL, manage_drones, new_drones[i]);

      #ifdef DEBUG
      printf("DRONE %d CREATED\n", new_drones[i]->id);
      #endif
    }

    drones = new_drones;
    drone_threads = new_drone_threads;
  }

  // Remove drones
  else {
    // Check if the number of drones can be reduced to given number
    int count_occupied = 0;
    for(int i = 0; i < n_drones; i++) {
      if(drones[i]->state) count_occupied++;
    }
    if(count_occupied > num_drones) {
      printf("TOO MANY ACTIVE DRONES\n");
      return;
    }

    // New arrays
    Drone **new_drones = (Drone**) malloc(num_drones*sizeof(Drone*));
    pthread_t **new_drone_threads = (pthread_t**) malloc(num_drones*sizeof(pthread_t*));

    // Put occupied drones in new arrays
    int count = 0;
    for(int i = 0; i < n_drones; i++) {
      if(drones[i]->state) {
        new_drones[i] = drones[i];
        new_drone_threads[i] = drone_threads[i];
        count++;
      }
    }

    // Put drones in array until full
    int i = 0;
    while(count < num_drones) {
      if(!drones[i]->state) {
        new_drones[i] = drones[i];
        new_drone_threads[i] = drone_threads[i];
        count++; i++;
      }
    }

    // Wait for remaining drones to finish
    for(; i < n_drones; i++) {
      if(!drones[i]->state) {
        pthread_cancel(*drone_threads[i]);
        pthread_join(*drone_threads[i], NULL);
        pthread_mutex_destroy(state_mutexes[i]);
        state_mutexes[i] = NULL;
        #ifdef DEBUG
        printf("DRONE %d JOINED\n", i+1);
        #endif
      }
    }

    drones = new_drones;
    drone_threads = new_drone_threads;
  }

  n_drones = num_drones;
  return;
}

void end_signal_handler(int signum) {
  close(fd);
  end_drones();
  for(int i = 0; i < drone_id-1; i++) {
    if(state_mutexes[i]) {
      pthread_mutex_destroy(state_mutexes[i]);
    }
  }
  pthread_cond_destroy(&state_cond);
  unlink(PIPE_LOCATION);
  exit(0);
}

// Process running the Central
int central_proc(int max_x, int max_y, int n_of_drones, Shm_Struct *shm, pnode_t* phead) {

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

  // Enables use of shared memory by Central and Drones
  shared_memory = shm;

  n_drones = n_of_drones;

  init_bases(max_x, max_y);

  drones = (Drone**) malloc(n_of_drones*sizeof(Drone*));
  if(init_drones()) {
    return 1;
  }

  create_pipe();
  while(1) {
    read_cmd(phead, max_x, max_y);
  }
}
