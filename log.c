#include "log.h"

void log_it(char *msg) {
  time_t raw_time;
  struct tm * time_info;

  time(&raw_time);
  time_info = localtime(&raw_time);

  char to_log[MAX_MSG_SIZE];

  sem_wait(log_sem);
  FILE *fp = fopen(LOG_LOCATION, "a");

  sprintf(to_log, "[%02d:%02d:%02d] %s\n", time_info->tm_hour, time_info->tm_min, time_info->tm_sec, msg);
  printf("%s", to_log);
  fprintf(fp, "%s", to_log);

  fclose(fp);
  sem_post(log_sem);
}
