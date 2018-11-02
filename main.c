#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/wait.h>
#include "sim_manager.h"
#include "central_proc.h"
#include "log.h"


int main(int argc, char const *argv[]){

  log_it("SIMULATION INITIATED");

    int * shared_var;
    int shmid;
    sem_t *mutex;

    double max_x, max_y;
    char* productlist;

	shmid = shmget(IPC_PRIVATE, __getpagesize(),IPC_CREAT|0700);
	shared_var = (int*) shmat(shmid, NULL, 0);

    pid_t process = fork();
    if (process == 0){
        central_proc(shmid);
        exit(0);
    } else {
      wait(NULL);
    }
    shmctl(shmid, IPC_RMID, NULL);

    log_it("SIMULATION TERMINATED");

    return 0;
}
