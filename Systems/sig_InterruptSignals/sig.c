/*  This is submission number 2. The purpose for this resubmission is only to
    include the functionality where each line inputted is echoed back line-by-
    line.
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/times.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#define HISTORY_LENGTH 10
#define SIZE 256
#define INTERVAL 10

//We use the following global variables since signals do not allow
//us to pass variables
int current_in;
int current_hlength;
int full;
char command_vault[HISTORY_LENGTH][SIZE];
int total_tick_timer = 0;

//Starts a timer that runs for INTERVAL seconds
//When the alarm rings, it will automatically start again for INTERVAL secs
void startalarm(){
  struct timeval interval_next,interval_cur;
  struct itimerval builder;
  interval_next.tv_sec = INTERVAL;
  interval_next.tv_usec = 0;
  interval_cur.tv_sec = INTERVAL;
  interval_cur.tv_usec = 0;
  builder.it_interval=interval_next;
  builder.it_value=interval_cur;
  const struct itimerval* mytimer = &builder;
  setitimer(ITIMER_REAL, mytimer, NULL);
}

void print_timespent(){
  struct tms *time_storage;
  time_storage = malloc(sizeof(struct tms));
  clock_t result = times(time_storage);
  double user_time,system_time;
  user_time = (double)(time_storage->tms_utime) / (double)CLOCKS_PER_SEC;
  system_time = (double)(time_storage->tms_stime) / (double)CLOCKS_PER_SEC;
  printf("user time: %g, system time: %g\n", user_time, system_time);
  free(time_storage);
}

void timespent_continue(){
  print_timespent();
}
void timespent_exit(){
  print_timespent();
  exit(0);
}

//Print the tick of INTERVAL seconds
void print_tick(int signum){
  ++total_tick_timer;
  fprintf(stderr,"tick %d...\n",(total_tick_timer * INTERVAL));
}

//Gets a line from stdin; only returns successful if it got a non-empty line
//Blank lines (containing \n) are okay though.
int getaline(char* buf) {
  if(!fgets(buf, SIZE, stdin)) return 0;
  if (strlen(buf) > 0)
    buf[strlen(buf)-1] = '\0';          //replace the ending \n with null char
  fprintf(stdout,"%s\n", buf);
  return 1;
}
void printcommands(){
  int i, current_out;                   //used for outputting string array
  if (!full) current_out=0;             //if not full, start output at oldest
  else current_out = current_in;        //otherwise, start output at current_in
  for (i=0; i < current_hlength; ++i) {
    if (full && (current_out == current_hlength))
      current_out = 0;                  //if we are at the end of ring, reset
    fprintf(stdout,"%s\n",command_vault[current_out]);
    ++current_out;
  }
}

int main() {
  struct sigaction new_action;
  //Establish our signal substitutions:
  // (accounting) SIGALRM (signal 14) will call 'print_tick'
  // 1. SIGINT (signal 2) will call 'timespent_continue'
  // 2. SIGTERM (signal 15) will call 'timespent_exit'
  // 3. SIGTSTP (signal 20) will call 'printcommands'
  new_action.sa_handler = print_tick;
  sigaction(SIGALRM, &new_action, NULL);  
  new_action.sa_handler = timespent_continue;
  sigaction(SIGINT, &new_action, NULL);
  new_action.sa_handler = timespent_exit;
  sigaction(SIGTERM, &new_action, NULL);
  new_action.sa_handler = printcommands;
  sigaction(SIGTSTP, &new_action, NULL);
  startalarm();

  current_in = 0, current_hlength = 0, full = 0;
  while (1) {
    if (current_in == HISTORY_LENGTH) { //if reached end of ring buffer
      current_in = 0;                   //reset head to start and mark as full
      full = 1;
    }
    //we're only interested if getaline returns a nonempty line
    if(getaline(command_vault[current_in])){
      ++current_in;
      if (!full) ++current_hlength;
    }
  }
}
