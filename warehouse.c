#include "warehouse.h"

wnode_t *this_wh;

void wh_end_signal_handler(int signum) {
  printf("%s SHUTTING DOWN\n", this_wh->name);
  exit(0);
}

void warehouse(int i, Shm_Struct *shm) {

  this_wh = &(shm->warehouses[i]);

  char msg[MSG_SIZE];
  sprintf(msg, "%s CREATED WITH PID %ld", this_wh->name, (long)getpid());
  log_it(msg);

  //###################SIGNAL HANDLING###############################

  // Central must ignore SIGUSR1 and SIGINT
  sigset_t block;
  sigemptyset(&block);
  sigaddset(&block, SIGUSR1);
  sigaddset(&block, SIGINT);
  sigprocmask(SIG_BLOCK, &block, NULL);

  // Central must handle SIGUSR2
  struct sigaction wh_end_action;
  wh_end_action.sa_handler = wh_end_signal_handler;
  sigemptyset(&wh_end_action.sa_mask);
  wh_end_action.sa_flags = 0;
  sigaction(SIGUSR2, &wh_end_action, NULL);

  //###################################################################

  if(i == 0) {
    shm->products_loaded += 2;
    this_wh->plist_head->quantity -= 2;
    printf("%s %s STOCK: %d\n", this_wh->name, this_wh->plist_head->name, this_wh->plist_head->quantity);
  }
  while(1) {

  }
  exit(0);
}
