#ifndef INCL_GUARD
#define INCL_GUARD

#include <sys/shm.h>
#include <string.h>

#define MSG_SIZE 100
#define REFILL_TYPE 1
#define DRONE_ARRIVAL_TYPE 2
#define DIE_TYPE 3

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
    int id;
    char* name;
    double chartx, charty;
    wpnode_t* plist_head;
} wnode_t;

// MQ message to warehouse structure
typedef struct {
  long wh_id;
  int msgtype;
  char *prod;
  int quantity, order_id;
} msg_to_wh;

// MQ message from warehouse structure
typedef struct {
  long order_id;
} msg_from_wh;

// Struct to hold shared memory IDs
typedef struct id_node {
  int id;
  struct id_node *next;
} id_node;

// Shared memory structure
typedef struct {
  int orders_given;
  int products_loaded;
  int orders_delivered;
  int products_delivered;
  double avg_time;
  int n_wh;
  wnode_t *warehouses;
} Shm_Struct;

int mq_id;
double time_unit;

void sim_manager(int max_x, int max_y, pnode_t *product_head, int n_of_drones, int refill_rate, int quantity, int n_of_whouses, wnode_t *whouses);
void usr_signal_handler(int signum);
void kill_signal_handler(int signum);
void print_statistics(Shm_Struct *shm);
id_node *create_shm();
void refill(int wh_id, int quantity);
pid_t create_central(int max_x, int max_y, int n_of_drones);
void create_warehouses();

#endif
