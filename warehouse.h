/******************************
Code created by:
   Diogo Ferrer, 2017247199
   Guilherme Cruz, 2016223968
*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/msg.h>
#include "log.h"
#include "sim_manager.h"

void wh_end_signal_handler(int signum);
void warehouse(int i, Shm_Struct *shm);
