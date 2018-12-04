#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include "log.h"
#include "central_proc.h"
#include <signal.h>
#include "sim_manager.h"
#include "warehouse.h"

// #define DEBUG

int shmid, n_wh;
Shm_Struct *shared_memory;
pid_t central, *wh_procs;
wnode_t *warehouses;
pnode_t *phead;

// Manages the simulation
void sim_manager(int max_x, int max_y, pnode_t *product_head, int n_of_drones, int refill_rate, int quantity, int n_of_whouses, wnode_t *whouses){

  //###################SIGNAL HANDLING###############################

  // Simulation manager must handle SIGUSR1 and SIGINT
  struct sigaction usr_action;
  usr_action.sa_handler = usr_signal_handler;
  sigemptyset(&usr_action.sa_mask);
  usr_action.sa_flags = 0;
  sigaction(SIGUSR1, &usr_action, NULL);

  struct sigaction kill_action;
  kill_action.sa_handler = kill_signal_handler;
  sigemptyset(&kill_action.sa_mask);
  kill_action.sa_flags = 0;
  sigaction(SIGINT, &kill_action, NULL);

  //###################################################################

  log_it("SIMULATION INITIATED");

  warehouses = whouses;

  n_wh = n_of_whouses;
  shmid = create_shm();

  mq_id = msgget(IPC_PRIVATE, IPC_CREAT|0700);

  phead = product_head;
  wh_procs = (pid_t*) malloc(n_wh * sizeof(pid_t));

  create_warehouses();

  central = create_central(max_x, max_y, n_of_drones);

  int i = 0;
  while(1) {
    int wh = (i++ % n_wh) + 1;
    refill(wh, quantity);
    sleep(time_unit * refill_rate);
  }
}

// Creates and initializes shared memory
int create_shm() {
  int shmid;
  shmid = shmget(IPC_PRIVATE, sizeof(Shm_Struct), IPC_CREAT|0700);
  shared_memory = (Shm_Struct*) shmat(shmid, NULL, 0);
  shared_memory->orders_given = 0;
  shared_memory->orders_delivered = 0;
  shared_memory->products_loaded = 0;
  shared_memory->products_delivered = 0;
  shared_memory->avg_time = 0.0;
  shared_memory->n_wh = n_wh;
  shared_memory->warehouses = warehouses;
  return shmid;
}

// Creates all the warehouse processes
void create_warehouses() {
  for(int i = 0; i < n_wh; i++) {
    wh_procs[i] = fork();
    if(wh_procs[i] == 0) {
      warehouse(i, shared_memory);
    }
  }
}

// Creates the Central process
pid_t create_central(int max_x, int max_y, int n_of_drones) {
  pid_t process = fork();
  if (process == 0){
      central_proc(max_x, max_y, n_of_drones, shared_memory, phead);
      exit(0);
  } else {
    return process;
  }
}

void refill(int wh_id, int quantity) {
  // Creates message to notify WH of refill
  msg_to_wh msg;
  msg.msgtype = REFILL_TYPE;
  msg.wh_id = (long)wh_id;

  // Count how many products there are in WH
  int count = 0;
  wpnode_t *plist = warehouses[wh_id-1].plist_head, *curr = plist;
  while(curr != NULL) {
    count++;
    curr = curr->next;
  }
  // Randomly choose one
  time_t t;
  srand((unsigned) time(&t));
  int prod = rand() % count;
  for(int i = 0; i < prod; i++) {
    plist = plist->next;
  }

  // Send message
  msg.prod = plist->name;
  msg.quantity = quantity;
  msgsnd(mq_id, &msg, sizeof(msg_to_wh), 0);
}

// Prints statistics stored in shared memory
void print_statistics(Shm_Struct *shm) {
  printf("\n-----------STATISTICS-----------\n");
  printf("TOTAL ORDERS GIVEN TO DRONES: %d\n", shm->orders_given);
  printf("TOTAL PRODUCTS LOADED: %d\n", shm->products_loaded);
  printf("TOTAL ORDERS DELIVERED: %d\n", shm->orders_delivered);
  printf("TOTAL PRODUCTS DELIVERED: %d\n", shm->products_delivered);
  printf("AVERAGE ORDER DELIVERY TIME: %.3fs\n", shm->avg_time);
  printf("--------------------------------\n\n");
}

// Handles SIGUSR1 by printing statistics
void usr_signal_handler(int signum) {
  print_statistics(shared_memory);
}

void kill_signal_handler(int signum) {
  log_it("SIGINT RECEIVED. TERMINATING SIMULATION...");
  kill(central, SIGUSR2);
  wait(NULL);
  for(int i = 0; i < 3; i++) {
    kill(wh_procs[i], SIGUSR2);
    wait(NULL);
  }
  shmctl(shmid, IPC_RMID, NULL);
  msgctl(mq_id, IPC_RMID, NULL);
  log_it("SIMULATION TERMINATED");
  exit(0);
}
