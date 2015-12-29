#include "help.h" 
#include <sys/stat.h>        /* open */
#include <signal.h>          /* sigaction */

#define TRUE  1
#define FALSE 0
#define INITIALTIMEOUT 500
#define MAXTIMEOUT 100000
#define INTERVAL 5

//Global bit array to store which blocks received
struct command loc_cmd; 
int shouldexit = FALSE;

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

void noserver(){
  if (shouldexit) {
    fprintf(stderr, "No Response From Server\n");
    exit(1);
  }
}

/* send a command to a server */ 
int my_send_command(int sockfd, const char *address, int port, 
                 const char *filename) { 
    struct command net_cmd; 
    struct sockaddr_in server_addr; 
/* set up cmd structure */ 
    strcpy(loc_cmd.filename, filename); 
/* translate to network order */ 
    command_local_to_network(&loc_cmd, &net_cmd) ; 
/* set up target address */ 
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = PF_INET;
    inet_pton(PF_INET, address, &server_addr.sin_addr);
    server_addr.sin_port        = htons(port);
/* send the packet: only send instantiated ranges */ 
    int sendsize = COMMAND_SIZE(loc_cmd.nranges);
    int ret = sendto(sockfd, (void *)&net_cmd, sendsize, 0, 
        (struct sockaddr *)&server_addr, sizeof(server_addr)); 
    if (ret<0) perror("send_command"); 
    return ret;
} 

int createfile(char *fname){
  int myfd;
  remove(fname);
  /* will fail if file does not exist, which is okay */
  myfd = open(fname, O_RDWR | O_CREAT, 0660);
  if (myfd < 0) {
    perror("open/create local (client) file");
    return 0;
  }
  return myfd;
}

int main(int argc, char **argv)
{
/* set up timer*/
  struct sigaction new_action;
  //Establish our signal substitution:
  new_action.sa_handler = noserver;
  sigaction(SIGALRM, &new_action, NULL);

/* local client data */ 
  int sockfd;		   		/* file descriptor for endpoint */ 
  struct sockaddr_in client_sockaddr;	/* address/port pair */ 
  struct in_addr client_addr;		/* network-format address */ 
  char client_dotted[INET_ADDRSTRLEN];	/* human-readable address */ 
  int client_port; 			/* local port */ 

/* remote server data */ 
  char *server_dotted; 			/* human-readable address */ 
  int server_port; 			/* remote port */ 

/* the request */ 
  char *filename; 			/* filename to request */ 

/* read arguments */ 
  if (argc != 5) { 
    fprintf(stderr, "client: wrong number of arguments\n"); 
    client_usage(); 
    exit(1); 
  } 
  server_dotted = argv[1]; 
  server_port = atoi(argv[2]); 
  client_port = atoi(argv[3]); 
  filename = argv[4]; 

  if (!client_arguments_valid(
    server_dotted, server_port, 
    client_port, filename)) { 
      client_usage(); 
      exit(1); 
  } 
/* get the primary IP address of this host */ 
  get_primary_addr(&client_addr); 
  inet_ntop(AF_INET, &client_addr, client_dotted, INET_ADDRSTRLEN);

/* construct an endpoint address with primary address and desired port */ 
  memset(&client_sockaddr, 0, sizeof(client_sockaddr));
  client_sockaddr.sin_family      = PF_INET;
  memcpy(&client_sockaddr.sin_addr,&client_addr,sizeof(struct in_addr)); 
  client_sockaddr.sin_port        = htons(client_port);

/* make a socket*/ 
  shouldexit = TRUE;                    /* Used for timer */
  starttimer();
  sockfd = socket(PF_INET, SOCK_DGRAM, 0);
  shouldexit = FALSE;
  if (sockfd<0) { 
    perror("can't open socket"); 
    exit(1); 
  } 

/* bind it to an appropriate local address and port */ 
  if (bind(sockfd, (struct sockaddr *) &client_sockaddr, 
          sizeof(client_sockaddr))<0) { 
    perror("can't bind local address"); 
    exit(1); 
  } 
  fprintf(stderr, "client: Receiving on %s, port %d\n", 
    client_dotted, client_port); 

/* send a command */ 
  send_command(sockfd, server_dotted, server_port, filename, 0, MAXINT); 
  fprintf(stderr, "client: requesting %s blocks %d-%d\n", 
                  filename, 0, MAXINT); 

/* client variables */
  uint32_t totalnumberofblocks = -1;
  int localfd;
  uint32_t written; //to delete later once func working
  int firstrun = TRUE;
  struct bits gotwhich;
  int done = FALSE; // set to TRUE when you think you're done
  int retval; 
  int i, windowsize;
  int bitarrayend = FALSE; /* determines end of checking bit array*/
  int timeout_msecs = INITIALTIMEOUT;
  int resendcount = 0;

  memset(&loc_cmd, 0, sizeof(loc_cmd)); 
  loc_cmd.nranges = 0;

/* receive the whole document and make naive assumptions */ 
  while (!done) { 
again: 
  /* TIMEOUT section, which you enter when no more blocks in recv q */ 
    if ((retval = select_block(sockfd, 0, timeout_msecs))==0
         && !firstrun) { 
      /* enter here if NOT 1st run, so use established gotwhich (bitarray)*/
      bitarrayend = FALSE;
      //fprintf(stderr, "HANDLE LACK OF RESPONSE HERE\n"); 
        if (totalnumberofblocks < 0){
        /* entering only means serious error, total#ofblocks not set*/
          perror("totalnumberofblocks");
          exit(1);  /* shouldn't get here */
        }
        if(bits_empty(&gotwhich)) {
        /* GETTING IN HERE MEANS WE'RE DONE */
          return 0; //ends the program, because we received our file!
        }
        /* will only get through here after totalnum is set (on first run)*/
        for(i=0 ; i<totalnumberofblocks; ++i){
          if(i == totalnumberofblocks - 1) {
            bitarrayend = TRUE;
          }
          windowsize = 0;
          /* one iteration for each block we have received */
          if(bits_testbit(&gotwhich, i)){
            /* in the bit array, 1 means unallocated, 0 means already received
             * thus, returning TRUE (1) means not there, MUST REFETCH window
               starting at: block i, with windowsize determining range.
             * for optomization, we see if 'next highest' is allocated
             * we build our ranges  up with windows until we run out of spots
               or until we reach the end of our array (bitarrayend) */
            while(bits_testbit(&gotwhich,i + windowsize + 1) &&
               ((i + windowsize + 1) < totalnumberofblocks)) { 
              /* entering this loop means next highest (wsize + 1)also missing
                 AND next highest (i+wsize+1) is less than total #
                 therefore once (i+wsize+1) ==total#, this condition skippd*/
              windowsize++;  
            }
            loc_cmd.ranges[loc_cmd.nranges].first_block = i;
            loc_cmd.ranges[loc_cmd.nranges].last_block = i + windowsize;
            loc_cmd.nranges++;

            /* at this point, we have some (up to MAXRANGES) ranges we want
             * my_send_command will rerequest it, so on server's attempt:
               1) will be resent & arrive in queue (which's why we goto:again)
               2) Attempt resend, and will get lost again, in which case
                  we'll be right back here, re-re-requesting it */
            if(loc_cmd.nranges == MAXRANGES || bitarrayend) {
              /* we stop looking for ranges and send the command once:
                 1) We have built up MAXRANGES (12) ranges, or
                 2) We have no more stuff that we need */
              my_send_command(sockfd, server_dotted, server_port,filename);
              loc_cmd.nranges = 0;
              goto again; /* restart so*/
            }
            i = i + windowsize; /* to continue for loop above window*/
          }
          else if (bitarrayend){
            my_send_command(sockfd, server_dotted, server_port,filename);
          }
        } //end of for loop that checks bitarray
    } else if (retval<0) { 
      /* error */ 
        perror("select"); 
        fprintf(stderr, "client: receive error\n"); 
    } else { 
      /* input is waiting, read it */ 
        struct sockaddr_in resp_sockaddr; 	/* address/port pair */ 
        int resp_len; 				/* length used */ 
        char resp_dotted[INET_ADDRSTRLEN]; 	/* human-readable address */ 
        int resp_port; 				/* port */ 
        int resp_mesglen; 			/* length of message */ 
        struct block one_block; 

      /* use helper routine to receive a block */ 
        shouldexit = TRUE;                      /* Used for timer */
        starttimer();
        recv_block(sockfd, &one_block, &resp_sockaddr); //GOT ONE_BLOCK!!
        shouldexit = FALSE;
        
        /**************************/
        /* when run first time, we finally have our first block */
        if(firstrun){
        /* will only run first run, initialize bitarray + create file*/
          totalnumberofblocks = one_block.total_blocks;
          if (!(localfd = createfile(one_block.filename)))
              exit(1);
          /* here we have filename created locally on client */
          /* set up bit array that keeps track of blocks we've received */
          bits_alloc(&gotwhich, totalnumberofblocks);
          bits_setrange(&gotwhich, 0, totalnumberofblocks-1);
          firstrun = FALSE;
        }
        /**************************/

	/* get human-readable internet address */
        inet_ntop(AF_INET, (void *)&(resp_sockaddr.sin_addr.s_addr),  
        resp_dotted, INET_ADDRSTRLEN);
        resp_port = ntohs(resp_sockaddr.sin_port); 

        /*fprintf(stderr, "client: %s:%d sent %s block %d (range 0-%d)\n",
                resp_dotted, resp_port, one_block.filename, 
                one_block.which_block, one_block.total_blocks);
*/
        /* check block data for errors */
        if (strcmp(filename, one_block.filename)!=0) { 
          fprintf(stderr,"client: received block w/ incorrect filename %s\n", 
                  one_block.filename); 
          goto again; 
        } 
	    
        if (bits_testbit(&gotwhich, one_block.which_block)){
        /* entering here means we do NOT yet have which_block recorded*/
          lseek(localfd, one_block.which_block * PAYLOADSIZE, SEEK_SET);
          /* write payload with paysize */
          written = write(localfd, one_block.payload, one_block.paysize);
          if (written != one_block.paysize){
            perror("Written");
            exit(1);
          }
          //keep track in bits with which block you got
          bits_clearbit(&gotwhich, one_block.which_block);
        }
        else {
          /* if we already had the block, then we know we were resent it
           * if we are resent 10 blocks, then our timeout can be dynamically
             adjusted by doubling it (up to MAXTIMEOUT) */
          resendcount++;
          if(resendcount > 10 && timeout_msecs < MAXTIMEOUT){
            timeout_msecs *= 2; //double it
            resendcount = 0;
          }
        }
      } 
  }
  close(localfd);
}
