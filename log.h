#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LOG_LOCATION "./log.txt"
#define MAX_CMD_SIZE 100

void log_it(char *msg) {
  char cmd[MAX_CMD_SIZE];
  sprintf(cmd, "echo `date +%%H:%%M:%%S` %s | tee -a %s", msg, LOG_LOCATION);
  system(cmd);
}
