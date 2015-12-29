#include <stdio.h>
#include <string.h> 
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h> 
#include <signal.h> 
#include <sys/time.h>

//#define HALLIGAN 1 /* if inside halligan, uncomment out to get IP printed */
#ifdef HALLIGAN
#include "/g/112/2012s/examples/Datagrams_and_Uncertainty/halligan.c"
#endif

#define BUF 4096
#define MAXNODES 255
#define INTERVAL 1
#define ACTIVE 1
#define INACTIVE 0

/* Global Structures*/
static struct timeval begin, end;
int firstrun = 0;
int sockfd_send;
char message[BUF];
int port;
struct sockaddr_in serv_addr_send;
struct servers_info {
  int number;
  char* IPs[MAXNODES];   /* an array (max 255 elements) of IP addresses */
  int active[MAXNODES];  /* an array of whether ip address is active/alive */
  struct timeval birth[MAXNODES]; /* used as a keepalive*/
} nodes;
/* Functions */
#ifdef HALLIGAN
void getprimary(int recv_port){
  /* get the primary IP address of this host */ 
  struct in_addr primary; 
  get_primary_addr(&primary); 
  char primary_dotted[INET_ADDRSTRLEN]; 
  inet_ntop(AF_INET, &primary, primary_dotted, INET_ADDRSTRLEN);
  fprintf(stderr, "Running on %s, port %d\n",	primary_dotted, recv_port); 
}
#endif
double sincestart(){
  gettimeofday(&end,NULL);
  return ((double)end.tv_sec + end.tv_usec/1000000.0)
       - ((double)begin.tv_sec + begin.tv_usec/1000000.0);
}
double age(nodenumber){
  gettimeofday(&end,NULL);
  return ((double)end.tv_sec + end.tv_usec/1000000.0)
       - ((double)nodes.birth[nodenumber].tv_sec
           + nodes.birth[nodenumber].tv_usec/1000000.0);
}

void starttimer(){
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
void displaynodes(){
  fprintf(stdout,"*** Active Servers ***\n");
  int k;
  for (k = 0; k < MAXNODES ;k++){
    /* If node is stale (over 5 seconds of age), make it inactive*/
    if(nodes.active[k] && age(k) > 5.0){
      /* means that registered as active but OLD (stopped rebroadcasting) */
      nodes.active[k] = INACTIVE;
      (nodes.number)--;
    }
    /* If active, display it*/
    if(nodes.active[k])
      fprintf(stdout, "%s\n",nodes.IPs[k]);
  }
}
int nodecached(char* cli_dotted){
  //if there and active, return 1
  //if there and inactive, make active and return 1
  //if not there, return 0
  if(nodes.number == 0)
    return 0;
  int k;
  for (k = 0; k < MAXNODES ;k++){
    if(!strcmp(nodes.IPs[k],cli_dotted)){
      //if enter here, then we already have the cli_dotted (active or not)
      if(nodes.active[k] == INACTIVE){    /* if inactive, activate it*/
        nodes.active[k] = ACTIVE;
        nodes.number++;
      }
      gettimeofday(&nodes.birth[k],NULL); /*reset birth*/
      return 1;
    }
    /* getting here means that the kth item in list is not cli_dotted, try ag*/
  }
  /* getting here means new entry (cli_dotted) not in list yet */
  return 0;
}
void broadcast(){
  int optval = 1;
  int returnval;
  double elapsed_d;
  int elapsed_i;
  
  if(!firstrun){
    /* Set up our sender */
    memset(&serv_addr_send, 0, sizeof(serv_addr_send));
    serv_addr_send.sin_family = PF_INET;
    serv_addr_send.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    serv_addr_send.sin_port = htons(port);
    sockfd_send = socket(PF_INET, SOCK_DGRAM, 0);
    returnval = setsockopt(sockfd_send, SOL_SOCKET, SO_BROADCAST, 
                         &optval, sizeof(optval));
    if(returnval != 0){
      perror("setsockopt problems\n");
      exit(1);
    }
    firstrun = 1;
  }
  /* SEND! (Broadcast!) */
  if (sendto(sockfd_send, message, strlen(message), 0,
        (struct sockaddr *)&serv_addr_send, sizeof(serv_addr_send))==(-1))
    perror("sendto");
 
  /* if five seconds has passed, displaynodes() */
  elapsed_d = sincestart();
  elapsed_i = (int)elapsed_d;
  if(elapsed_i % 5 == 0){       /* see if 5 seconds has passed */
    displaynodes();
  }
}
void init(){
  /* Initialize our time, as well as our nodes' structure */
  gettimeofday(&begin, NULL);
  nodes.number = 0;
  int i;
  for(i = 1; i < MAXNODES; i++){
    nodes.active[i] = INACTIVE;
    nodes.IPs[i] = malloc(sizeof("255.255.255.255"));
  }
}
int main(int argc, char** argv){
  if (argc != 2) {
    fprintf(stderr,"%s usage: %s port\n", argv[0], argv[0]);
    exit(1);
  }
  init();
  /* Establish our signal substitution */
  struct sigaction new_action;
  new_action.sa_handler = broadcast;
  sigaction(SIGALRM, &new_action, NULL);
  starttimer();                 /* set up timer*/
  port = atoi(argv[1]);         /* set global variable 'port' */
  /* Note the following message can be used in the future.
     One idea is to ^ delimit the string with active nodes' IPs */
  strcpy(message,"ForFutureUse");
#ifdef HALLIGAN
  getprimary(port);             /* prints information about current IP/port */
#endif

  /* -- Listener -- */
  int sockfd;
  /* server info */
  struct sockaddr_in serv_addr;
  /* incoming message info */
  int msglen;
  char incoming_message[BUF];   /* for future use*/
  /* client info */
  struct sockaddr_in cli_addr;
  int cli_len;
  char cli_dotted[BUF];
  int serv_port = 0;
  serv_port=atoi(argv[1]);
  if (serv_port<9000 || serv_port>32767) {
    fprintf(stderr, "%s: port %d is not allowed\n", argv[0],
    serv_port); exit(1);
  }
  /* make a socket */
  sockfd = socket(PF_INET, SOCK_DGRAM, 0);
  /* make up a socket address */
  memset(&serv_addr, 0 , sizeof(serv_addr));
  serv_addr.sin_family      = PF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port        = htons(serv_port);
  /* bind it to an address and port */
  if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
    perror("can't bind local address");
    exit(1);
  }
  for ( ; ;) {
    /* get a datagram */
    cli_len = sizeof(cli_addr);
    msglen = recvfrom(sockfd, incoming_message, BUF, 0,
               (struct sockaddr *) &cli_addr, &cli_len);
    if (msglen >= 0) {
      /* get numeric internet address */
      inet_ntop(PF_INET, (void *)&(cli_addr.sin_addr.s_addr),
                   cli_dotted, BUF);
      if(!nodecached(cli_dotted)){
        /* first check if we are over out limit */
        if(nodes.number >= MAXNODES - 1){
          fprintf(stderr,"Max number of nodes reached, cannot add another\n");
          break;
        }
        /* nodecached: 1 means there and active, 0 means not there */
        nodes.IPs[nodes.number] = malloc(sizeof(cli_dotted));
        strcpy(nodes.IPs[nodes.number], cli_dotted);
        nodes.active[nodes.number] = ACTIVE;
        gettimeofday(&nodes.birth[nodes.number],NULL);
        nodes.number++;
      }
    }
    else { /* expected when sigalarm rings, no action needed */ }
  }
}
