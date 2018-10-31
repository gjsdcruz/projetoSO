#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sim_manager.h>


int main(int argc, char const *argv[]){

    int * shared_var;
    int shmid;
    sem_t *mutex;
    
    int max_x, max_y;
    char* productlist[];

	shmid = shmget(IPC_PRIVATE, __getpagesize(),IPC_CREAT|0700);
	shared_var = (int*) shmat(shmid, NULL, 0);

    pid_t process = fork();
    if (process == 0){
        central_proc(shmid);
    } else {
        
    }
    return 0;
}
