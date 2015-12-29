/***************************************
watch.c can be compiled with: gcc -lpthread watch.c -o watch
usage: ./watch <executableprogram>

Objective #1 (addressed in the function child_reaper):
  . A report is printed every time the child (or <executableprogram>) dies.
  This could be from either exiting normally, reaching a resource limit (and
  get killed by OS), or be killed by part of the watch program.
Objective #2 (#2-#4 are addressed in monitor loop in main):
  . By checking the information in /proc, watch is able to determine if more
  than 4MB of memory is taken, and the child is killed and reported. With
  (optional) resource limits in place, the watch program will not specifically
  report that the child was killed, however the report (in #1) will be printed.
  The resource limits are considered optional because the monitor loop will
  nonetheless cover the requirements (with slightly more leniency due to the
  nature of monitoring in a loop, and therefore allowing additional work
  to be done).
Objective #3:
  . By checking the information in /proc, watch is able to determine if more
  than 1 second of CPU time is taken. A resource limit is imposed, and the
  comments in #3 apply.
Objective #4:
  . As each line the child prints on stdout is received and redirected, watch
  will monitor whether total lines redirected ever reaches 1000, at which point
  output redirection is ceased. The monitoring loop will subsequently identify
  this and kill the child.
In cases 2-4, a SIGKILL is sent (since children cannot ignore). Additionally,
  it is possible that a violating child exits before the violation can be
  noted and the KILL sent.
****************************************/
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h> 
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "/comp/111/assignments/a3/proc.c"

#define SIZE 4096
#define READER 0
#define WRITER 1
#define MAX_LINES 1000
#define FOURMB 4194304

//Globals
pthread_t thread;
int num_lines_read = 0;

struct inputdata{
  FILE* infile;
  int childpid;
};

struct outputdata{
  int nodata;
} thread_output;          //needs to be global to outlast child

void child_reaper(int sig) {
  int status;
  struct rusage usage;
  int cpid=wait3(&status, WNOHANG, &usage);
  if (cpid <= 0) {        //Would like to prevent against child sending SIGCHLD
    fprintf(stderr,"Child not dead, maybe you've been duped!");
    return;
  }
  //Join the thread, will block here until reading from child is done
  pthread_join(thread,NULL);  //NULL return ptr because global var output

  int timeofday;
  struct timeval current_time;
  timeofday = gettimeofday(&current_time,NULL);
  struct tm* ptm;
  char time_string[40];
  ptm = localtime (&current_time.tv_sec); 
  /* Format the date and time. */ 
  strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm); 

  // ==> Objective #1 <==
  fprintf(stderr,"==========> REPORT <==========\n");
  fprintf(stderr,"Child (PID %d) died\n",cpid);
  fprintf(stderr,"Total runtime: %ld sec, %ld usec\n",
                 usage.ru_utime.tv_sec,usage.ru_utime.tv_usec);
  fprintf(stderr,"Lines printed to stdout: %d\n",num_lines_read);
  fprintf(stderr,"Time of death: %s\n", time_string); 
  fprintf(stderr,"==============================\n");
  exit(0);
}

//Thread accepts input from pipe, which is output from child
void *threaded_reader(void *v){
  int count =0, alive = 0;
  struct inputdata *d = (struct inputdata*) v;
  alive =1;
  while (alive) {
    char* result = NULL, *newline_ptr = NULL;
    int bufsize = SIZE;
    if(getline(&result,&bufsize,d->infile) != -1) {
      //Formatting: Get rid of extra new lines accepted
      if(newline_ptr = strchr(result,'\n'))
        *newline_ptr = '\0';
      //Once we've read enough lines, break to stop redirection
      if (num_lines_read == MAX_LINES)
        break;
      //Now we actually redirect childs output to stdout
      fprintf(stdout,"%s\n",result);
      num_lines_read++;
    }
    else alive = 0;
  }
  close(fileno(d->infile));
  return (void*) &thread_output;            //don't do anything with output
}

int main(int argc, char** argv){
  int pid;
  int fd[2];
  if (argc !=2) {
    fprintf(stderr,"usage: %s <executableprogram>\n",argv[0]);
    exit(1);
  }

  pipe(fd);
  if (pid=fork())  { //parent!
    int alive_2 = 1, alive_3 = 1, alive_4 = 1;
    int page_size = getpagesize();
    int stack_used, CPU_used;
    struct inputdata input;
    struct sigaction new_action;
    //Set up process info and process info m
    struct statStuff procStat;
    struct statmStuff procStatm;

    //Use reader end of pipe (reading child's output) as input
    FILE *readfile = fdopen(fd[READER],"r");
    close(fd[WRITER]);                      //Close writer since we don't use
    if (readfile == NULL) {
      perror("Cannot dup, exiting.");
      exit(1);
    }
    //Set up input struct to give to thread with file (pipe) and child's pid
    input.infile = readfile;
    input.childpid = pid;
    if(pthread_create(&thread, NULL, threaded_reader,(void*)&input) != 0) {
      fprintf(stderr,"Could not create thread, exiting.\n");
      exit(1);
    }
    //Set up signal handler for SIGCHLD
    memset(&new_action,0,sizeof(struct sigaction));
    new_action.sa_handler = child_reaper;
    sigaction(SIGCHLD, &new_action, NULL);

    // ==> MONITOR <==
    while(1) {
      readStat(pid,&procStat);
      readStatm(pid, &procStatm);
      // ==> Objective #1 <==
        //Covered in function child_reaper
      // ==> Objective #2 <==
      //use proc to determine 4MB of stack space
      stack_used = (procStatm.data * page_size);
      if (stack_used > FOURMB && alive_2) {
        alive_2 = 0;
        if (kill(pid,SIGKILL) == 0)
          fprintf(stderr,"STACK limit (4MB) reached; killing child\n");
      }
      // ==> Objective #3 <==
      CPU_used = procStat.utime / sysconf(_SC_CLK_TCK);
      if (CPU_used == 1 && alive_3){
        alive_3 = 0;
        if(kill(pid,SIGKILL) == 0)
          fprintf(stderr,"CPU limit (1 sec) reached; killing child\n");
      }
      // ==> Objective #4 <==
      if (num_lines_read >= 1000 && alive_4){
        alive_4 = 0;
        if(kill(pid,SIGKILL) == 0)
          fprintf(stderr,"Output lines limit (1000) reached; killing child\n");
      }
    }
  }
  else {                           //child
    close(fileno(stdout));
    dup(fd[WRITER]);               //writing end of pipe takes over
    close(fd[WRITER]);             //close old pipe writer
    close(fd[READER]);             //we don't need it

                  /* +++ OPTIONAL (see notes in header) +++  */
    struct rlimit CPU_limit, STACK_limit;
    CPU_limit.rlim_cur = 1;
    CPU_limit.rlim_max = 1;                      //Hard limit 1sec CPU time
    if(setrlimit(RLIMIT_CPU, &CPU_limit)){       //return 0 on success
      fprintf(stderr,"Setting RLIMIT_CPU resource failed\n");
      exit(1);
    }
    STACK_limit.rlim_cur = FOURMB;
    STACK_limit.rlim_max = FOURMB;               //Hard limit 4MB stack space
    if(setrlimit(RLIMIT_STACK, &STACK_limit)){   //return 0 on success
      fprintf(stderr,"Setting RLIMIT_STACK resource failed\n");
      exit(1);
    }
                  /* --- OPTIONAL (see notes in header) ---  */

    execl(argv[1],argv[1],NULL);                 //this should write to pipe!
  }
}
