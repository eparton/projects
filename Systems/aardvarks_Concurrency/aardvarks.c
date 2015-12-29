#include "/comp/111/assignments/a4/anthills.h" 

//Globals needed
int ready = 0;
struct inputdata{
  int hillno;
};

//State Mutex
static pthread_mutex_t state_lock;

//Global State Structure
struct state_machine {
  int hill_activity[ANTHILLS];          //# of aardvarks at a given hill (0-2)
  int remaining_ants[ANTHILLS];         //# of ants remaining at given hill
} sm;

//initialize mutex and populate anthills
void initialize(){
  pthread_mutex_init(&state_lock, NULL); 
  int j;
  for (j=0; j < ANTHILLS ; j++)
    sm.remaining_ants[j] = ANTS_PER_HILL;
}
//finds and returns an available hill number
//returns -1 if either none available,
//  or if hills differ in more than 'advantage' number of ants
int available_hill(){
  int j, hill_choice;
  int advantage = 2;
  hill_choice = lrand48() % ANTHILLS;    //will give me a random 0-2
  for (j=0; j < ANTHILLS ; j++){
    if (sm.hill_activity[hill_choice] != AARDVARKS_PER_HILL
        && sm.remaining_ants[hill_choice] > 0){
      //if some anthill has 2 fewer than the others, don't use it!
      if ((sm.remaining_ants[hill_choice] + advantage)
               <= sm.remaining_ants[(hill_choice+1)%ANTHILLS]
            || (sm.remaining_ants[hill_choice] + advantage) 
               <= sm.remaining_ants[(hill_choice+2)%ANTHILLS]){
        return -1;
      }
      return hill_choice;
    }
  }
  return -1;
}
//times for a little over a second, then frees up a spot at given anthill
void *time_and_release(void *in){
  struct inputdata *hill_data = (struct inputdata*) in;
  usleep(1010000);
  (sm.hill_activity[hill_data->hillno])--;
}

void update(int hillno){
  //don't need mutex in here, because update() is inside critical section.
  (sm.hill_activity[hillno])++;          //records one more aardvark at hill
  (sm.remaining_ants[hillno])--;         //takes one ant away (eaten!)
}

void my_slurp(char aname){
  int hill_request;
  //if all anthills completely empty, return with no slurping activity
  if(sm.remaining_ants[0] == 0 && sm.remaining_ants[1] == 0
                               && sm.remaining_ants[2] == 0)
    return;
  pthread_mutex_lock(&state_lock);
  hill_request = available_hill();
  if(hill_request < 0) {
    pthread_mutex_unlock(&state_lock);
    return;
  }
  update(hill_request);
  //start in a thread a timer that runs a little over a second;
  //then release a spot at given hill
  pthread_t thread;
  struct inputdata input;
  input.hillno = hill_request;
  if(pthread_create(&thread, NULL, time_and_release,(void*)&input) != 0) {
    fprintf(stderr,"Could not create thread, exiting.\n");
    exit(1);
    }
  pthread_mutex_unlock(&state_lock); 
	
  if(!slurp(aname,hill_request))         //returns false if did not slurp
    (sm.remaining_ants[hill_request])++; //must add an ant if slurp failed
  pthread_join(thread,NULL);
  return;
}

void *thread_A(void *input) { 
    initialize();                //initialize the mutex, and the hills
    ready = 1;          //don't let other aardvarks start until initialize done
    char aname = *(char *)input; // name of aardvark, for debugging
    while (chow_time()) my_slurp(aname);
    pthread_mutex_destroy(&state_lock); //Must destroy the mutex
                                 //Safe to do that here because chow_time()
                                 //won't return FALSE until all anthills empty
    return NULL; 
} 
void *thread_B(void *input) { 
    while(!ready) {}
    char aname = *(char *)input; // name of aardvark, for debugging
    while (chow_time()) my_slurp(aname);
    return NULL; 
} 
void *thread_C(void *input) { 
    while(!ready) {}
    char aname = *(char *)input; // name of aardvark, for debugging
    while (chow_time()) my_slurp(aname); 
    return NULL; 
} 
void *thread_D(void *input) { 
    while(!ready) {}
    char aname = *(char *)input; // name of aardvark, for debugging
    while (chow_time()) my_slurp(aname); 
    return NULL; 
} 
void *thread_E(void *input) { 
    while(!ready) {}
    char aname = *(char *)input; // name of aardvark, for debugging
    while (chow_time()) my_slurp(aname); 
    return NULL; 
} 
void *thread_F(void *input) { 
    while(!ready) {}
    char aname = *(char *)input; // name of aardvark, for debugging
    while (chow_time()) my_slurp(aname); 
    return NULL; 
} 
void *thread_G(void *input) { 
    while(!ready) {}
    char aname = *(char *)input; // name of aardvark, for debugging
    while (chow_time()) my_slurp(aname);
    return NULL; 
} 
void *thread_H(void *input) { 
    while(!ready) {}
    char aname = *(char *)input; // name of aardvark, for debugging
    while (chow_time()) my_slurp(aname); 
    return NULL; 
} 
