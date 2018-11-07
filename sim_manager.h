#ifndef INCL_GUARD
#define INCL_GUARD

#include <sys/shm.h>

// Product node struct
typedef struct pnode {
    char* name;
    struct pnode* next;
} pnode_t;

// Warehouse product node struct
typedef struct wpnode {
    char* name;
    int quantity;
    struct wpnode* next;
} wpnode_t;

// Warehouse node struct
typedef struct wnode {
    char* name;
    double chartx, charty;
    wpnode_t* plist_head;
} wnode_t;

// Shared memory structure
typedef struct {
  int orders_given;
  int products_loaded;
  int orders_delivered;
  int products_delivered;
  double avg_time;
} Shm_Struct;

void sim_manager(int max_x, int max_y, pnode_t *product_head, int n_of_drones, int refill_rate, int quantity, int time_unit, int n_of_whouses);
void usr_signal_handler(int signum);
void print_statistics(Shm_Struct *shm);
int create_shm();
pid_t create_central(int max_x, int max_y, int n_of_drones);

#endif
