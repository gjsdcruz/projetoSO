#include "warehouse.h"

#define DEBUG

wnode_t *this_wh;

void wh_end_signal_handler(int signum) {
  char log_msg[MSG_SIZE];
  sprintf(log_msg, "%s WITH PID %ld CLOSED", this_wh->name, (long)getpid());
  log_it(log_msg);
  exit(0);
}

void warehouse(int i, Shm_Struct *shm) {

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

  this_wh = &(shm->warehouses[i]);

  char log_msg[MSG_SIZE];
  sprintf(log_msg, "%s CREATED WITH PID %ld", this_wh->name, (long)getpid());
  log_it(log_msg);

  while(1) {
    // Read message from MQ
    msg_to_wh msg;
    msgrcv(mq_id, &msg, sizeof(msg_to_wh), (long)this_wh->id, 0);

    // Separate drone arrivals from refills
    if(msg.msgtype == DRONE_ARRIVAL_TYPE) {
      // Process order and send message when ready
      #ifdef DEBUG
      printf("%s READY TO LOAD DRONE\n", this_wh->name);
      #endif

      sleep(msg.quantity * time_unit);

      shm->products_loaded += msg.quantity;
      msg_from_wh out_msg;
      out_msg.order_id = (long)msg.order_id;
      msgsnd(mq_id, &out_msg, sizeof(msg_from_wh), 0);

      #ifdef DEBUG
      printf("%s FINISHED LOADING DRONE\n", this_wh->name);
      #endif
    }
    else if(msg.msgtype == REFILL_TYPE) {
      // Find product to refill
      wpnode_t *curr = this_wh->plist_head;
      while(curr && strcmp(curr->name, msg.prod)) {
        curr = curr->next;
      }
      // Update stock
      curr->quantity += msg.quantity;

      char log_msg[MSG_SIZE];
      sprintf(log_msg, "%s WAS REFILLED", this_wh->name);
      log_it(log_msg);
    }
    else {
      wh_end_signal_handler(0);
    }
  }
}
