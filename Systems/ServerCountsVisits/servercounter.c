#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>

#define BUFSIZE 4096
#define LISTENQ 10
#define MAXBROWSERS 10

int main(int argc, char** argv) {
  int sockfd, newsockfd;              //file descriptors
  int bindreturn;
  int addrlen;
  int readreturn, writereturn;        //return values from reading/writing
  char readbuf[BUFSIZE];
  char writebuf[BUFSIZE];             //a buffer to write into
  char *tempwrite;
  int browsercount = 0;               //count how many browsers found
  int browsercountmod;                //browser count mod'ed by MAXBROWSERS
  struct sockaddr_in server_info;     //this guy - used for bind()
  struct sockaddr_in client_info;     //other guy - used for accept()
  int portnum;

  if (argc != 2){
    fprintf(stderr,"Usage: %s <portnumber>\n",argv[0]);
    exit(1);
  }
  portnum = atoi(argv[1]);

  //Put information into server_info
  memset((char *) &server_info,0, sizeof(server_info)); //zero out first
  server_info.sin_family = AF_INET;        //so that we use IPv4 (not v6)
  server_info.sin_port = htons(portnum);   //uses port in right format
  server_info.sin_addr.s_addr = INADDR_ANY;             //doesn't need to know
                                                        //IP of machine run on
  //establish socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Socket");
    exit(1);
  }
  //bind to a socket
  bindreturn = bind(sockfd, (struct sockaddr*) &server_info,
                              sizeof(server_info));
  if (bindreturn < 0) {
    perror("Bind");
    exit(1);
  }
  //listen with a queue of 10
  listen(sockfd, LISTENQ);
  printf("Listening on port %d\n", portnum);
  //start infinite loop (servers loop infinitely until killed)
  for (;;) {
    //accept a socket
    addrlen = sizeof(client_info);
    newsockfd = accept(sockfd, (struct sockaddr*)&client_info,&addrlen);
    if (newsockfd < 0){
      perror("Accept");
      exit(1);
    }
    //read from connection
    readreturn = read(newsockfd, readbuf, BUFSIZE);
    if (readreturn < 0) {
      perror("Read");
      exit(1);
    }
    readbuf[readreturn] = '\0';
    //check for a corrupt header that includes: >, <, or "
    int corruptheader = 0;
    if(strchr(readbuf, '<') || strchr(readbuf, '>') || strchr(readbuf, '"'))
      corruptheader = 1;

    //** <Extra Credit>
    //examine first line of header (work begun for extra credit)
    size_t firstlen;
    char firstline[BUFSIZE];
    char *slash;
    char *aboutme = "/aboutme \0";
    int otherpage = 0;
    firstlen = strcspn(readbuf, "\n");
    strncpy(firstline, readbuf, firstlen);
    //printf("first line: %s\n", firstline);
    slash = strchr(firstline, '/');
    if(!strncmp(slash,aboutme,strlen(aboutme))){
      otherpage = 1;
    }
    // </Extra Credit> **/

    //getting here, we have read something; we will check if it is a browser
    char *description;           //stores location of string with browser desc
    char *useragent = "User-Agent:";
    //keep an array of strings of browser descritions
    char browserdetails[MAXBROWSERS][BUFSIZE];
    description = strstr(readbuf,useragent);
    if (description &&
                       !otherpage){ //ExtraCredit
      int j;
      //if we have previously found a corrupt header, put censor in desc array
      if (corruptheader){
        browsercountmod = browsercount % MAXBROWSERS;
        strcpy(browserdetails[browsercountmod],
               "CENSORED for security reasons");
      }
      //else store the actual browser desc
      else{
        for (j = 0; description[strlen(useragent) + 1 + j] != '\n'; j++) {
          browsercountmod = browsercount % MAXBROWSERS;
          browserdetails[browsercountmod][j] =
                         description[strlen(useragent) + 1 + j];
        }
        browserdetails[browsercountmod][j] = '\0';
      }
      browsercount++;
    }
    else {
      //Not a browser OR aboutme requested: do nothing!
    }

    //write to connection
    tempwrite = "HTTP/1.1 200 OK\n";
    strcpy(writebuf, tempwrite);
    writereturn = write(newsockfd, writebuf, strlen(writebuf));
    if (writereturn < 0) perror("Write");
    tempwrite = "Content-type: text/html\n\n";
    strcpy(writebuf, tempwrite);
    writereturn = write(newsockfd, writebuf, strlen(writebuf));
    if (writereturn < 0) perror("Write");

    //return browser details
    int k;
    int iternum;
    int tail = 0, head = 0, len = 0;
    char tempbrowser[BUFSIZE];
    strcpy(tempbrowser,"\0");
    char numberaschar[BUFSIZE];
    if (browsercount >= MAXBROWSERS){
      tail = (browsercount) % MAXBROWSERS;
    }
    head = (browsercount -1) % MAXBROWSERS;

    //iterate through output and give a line for each browser description
    for(k = 0; k < browsercount && k < MAXBROWSERS; ++k) {
      if (browsercount < MAXBROWSERS)
        iternum = browsercount - k;
      else
        iternum = MAXBROWSERS - k;
      //Convert iteration number to a char
      if(snprintf(numberaschar, BUFSIZE, "%d", iternum) == -1) {
        perror("snprintf");
        exit(1);
      }
      strcat(tempbrowser, numberaschar);
      strcat(tempbrowser, ") ");
      strcat(tempbrowser, browserdetails[(tail + k) % MAXBROWSERS]);
      strcat(tempbrowser, "<br>");      //gives a newline in html
      strcpy(writebuf, tempbrowser);
    }
    //ExtraCredit +
    char *aboutmelink="<br><a href=\"aboutme\">About Me!</a>";
    if (otherpage){ 
       char *allaboutme = "<img src=\"http://jerseyjoeart.files.wordpress.com/2009/12/tmnt_cowabunga.jpg\"/><a href=\"home\">Back Home</a>";
      strcpy(writebuf, allaboutme);
    } 
    else
      strcat(writebuf, aboutmelink);
    //ExtraCredit -
    writereturn = write(newsockfd, writebuf, strlen(writebuf));
    if (writereturn < 0) perror("Write");

    //close connection (newsockfd), and do it inside this loop (on each iter)
    close(newsockfd);
  }
  close(sockfd);
}
