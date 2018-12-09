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

int shmid, wh_shm_id, n_wh;
Shm_Struct *shared_memory;
pid_t central, *wh_procs;
wnode_t *warehouses;
pnode_t *phead;
id_node *id_list;

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

  // Simulation manager must block all other signals except SIGUSR2 which will be needed by other processes
  sigset_t block;
  sigfillset(&block);
  sigdelset(&block, SIGINT);
  sigdelset(&block, SIGUSR1);
  sigdelset(&block, SIGUSR2);
  sigprocmask(SIG_BLOCK, &block, NULL);

  //###################################################################

  // Initialize log semaphore before logging beginning of simulation
  sem_unlink("LOG");
  log_sem = sem_open("LOG", O_CREAT|O_EXCL, 0700, 1);

  log_it("SIMULATION INITIATED");

  warehouses = whouses;

  n_wh = n_of_whouses;

  // Initialize shared memory semaphore to synchronize future accesses
  sem_unlink("SHM");
  shm_sem = sem_open("SHM", O_CREAT|O_EXCL, 0700, 1);
  id_list = create_shm();

  // Create message queue to communicate between processes
  mq_id = msgget(IPC_PRIVATE, IPC_CREAT|0700);

  phead = product_head;
  wh_procs = (pid_t*) malloc(n_wh * sizeof(pid_t));
  create_warehouses();

  central = create_central(max_x, max_y, n_of_drones);

  // At refill rate, select a warehouse using round robin algorithm
  // and send refill message to selected warehouse
  int i = 0;
  while(1) {
    int wh = (i++ % n_wh) + 1;
    refill(wh, quantity);
    usleep(time_unit * refill_rate * 1000000);
  }
}

// Creates and initializes shared memory
id_node *create_shm() {
  shmid = shmget(IPC_PRIVATE, sizeof(Shm_Struct), IPC_CREAT|0700);
  shared_memory = (Shm_Struct*) shmat(shmid, NULL, 0);
  shared_memory->orders_given = 0;
  shared_memory->orders_delivered = 0;
  shared_memory->products_loaded = 0;
  shared_memory->products_delivered = 0;
  shared_memory->avg_time = 0.0;
  shared_memory->n_wh = n_wh;
  // Copy warehouses data to shared memory
  wh_shm_id = shmget(IPC_PRIVATE, n_wh*sizeof(wnode_t), IPC_CREAT|0700);
  shared_memory->warehouses = (wnode_t*) shmat(wh_shm_id, NULL, 0);
  id_node *id_list = NULL, *curr_id = NULL;
  for(int i = 0; i < n_wh; i++) {
    shared_memory->warehouses[i].id = warehouses[i].id;
    shared_memory->warehouses[i].name = (char*) malloc(WORD_SIZE * sizeof(char));
    strcpy(shared_memory->warehouses[i].name, warehouses[i].name);
    shared_memory->warehouses[i].chartx = warehouses[i].chartx;
    shared_memory->warehouses[i].charty = warehouses[i].charty;

    // Create linked list of IDs and shared memory pieces containing each warehouse product and its stock
    shared_memory->warehouses[i].plist_head = NULL;
    wpnode_t *aux = warehouses[i].plist_head, *curr = shared_memory->warehouses[i].plist_head;
    while(aux) {
      id_node *new_id = (id_node*) malloc(sizeof(id_node));
      new_id->id = shmget(IPC_PRIVATE, sizeof(wpnode_t), IPC_CREAT|0700);
      new_id->next = NULL;

      if(!id_list) {
        id_list = new_id;
        curr_id = new_id;
      }

      else {
        curr_id->next = new_id;
        curr_id = new_id;
      }

      wpnode_t *new_prod = (wpnode_t*) shmat(new_id->id, NULL, 0);
      new_prod->name = (char*) malloc(sizeof(char)*WORD_SIZE);
      strcpy(new_prod->name, aux->name);
      new_prod->quantity = aux->quantity;
      new_prod->next = NULL;

      if(!shared_memory->warehouses[i].plist_head) {
        shared_memory->warehouses[i].plist_head = new_prod;
        curr = new_prod;
      }

      else {
        curr->next = new_prod;
        curr = curr->next;
      }

      aux = aux->next;
    }
  }

  return id_list;
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
  sem_wait(shm_sem);
  printf("\n-----------STATISTICS-----------\n");
  printf("TOTAL ORDERS GIVEN TO DRONES: %d\n", shm->orders_given);
  printf("TOTAL PRODUCTS LOADED: %d\n", shm->products_loaded);
  printf("TOTAL ORDERS DELIVERED: %d\n", shm->orders_delivered);
  printf("TOTAL PRODUCTS DELIVERED: %d\n", shm->products_delivered);
  printf("AVERAGE ORDER DELIVERY TIME: %.3fs\n", shm->avg_time);
  printf("--------------------------------\n\n");
  sem_post(shm_sem);
}

// Handles SIGUSR1 by printing statistics
void usr_signal_handler(int signum) {
  print_statistics(shared_memory);
}

void kill_signal_handler(int signum) {
  log_it("SIGINT RECEIVED. TERMINATING SIMULATION...");

  // Signal Central to shut down and wait
  kill(central, SIGUSR2);
  wait(NULL);

  // Send message to warehouses telling them to close
  msg_to_wh die_msg;
  die_msg.msgtype = DIE_TYPE;
  for(int i = 0; i < n_wh; i++) {
    die_msg.wh_id = i+1;
    msgsnd(mq_id, &die_msg, sizeof(msg_to_wh), 0);
    wait(NULL);
  }

  // Release resources
  id_node *curr = id_list, *prev = NULL;
  while(curr) {
    shmctl(curr->id, IPC_RMID, NULL);
    prev = curr;
    curr = curr->next;
    free(prev);
  }
  shmctl(wh_shm_id, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  msgctl(mq_id, IPC_RMID, NULL);

  log_it("SIMULATION TERMINATED");

  // Remove log and shared memory semaphores
  sem_close(log_sem);
  sem_unlink("LOG");
  sem_close(shm_sem);
  sem_unlink("SHM");
  exit(0);
}
