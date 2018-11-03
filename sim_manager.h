#ifndef INCL_GUARD
#define INCL_GUARD

#include <sys/shm.h>

// Shared memory structure
typedef struct {
  int orders_given;
  int products_loaded;
  int orders_delivered;
  int products_delivered;
  double avg_time;
} Shm_Struct;

void sim_manager();
void usr_signal_handler(int signum);
void print_statistics(Shm_Struct *shm);
int create_shm();
pid_t create_central();

#endif
