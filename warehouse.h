#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
// #include <sys/types.h>
#include <unistd.h>
#include "log.h"
#include "sim_manager.h"

#define MSG_SIZE 100

void wh_end_signal_handler(int signum);
void warehouse(int i, Shm_Struct *shm);
