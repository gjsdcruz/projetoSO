#include "log.h"

void log_it(char *msg) {
  char cmd[MAX_CMD_SIZE];
  sprintf(cmd, "echo `date +%%H:%%M:%%S` %s | tee -a %s", msg, LOG_LOCATION);
  system(cmd);
}
