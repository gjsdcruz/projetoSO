#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "sim_manager.h"

void wh_end_signal_handler(int signum);
void warehouse(int i, Shm_Struct *shm);
