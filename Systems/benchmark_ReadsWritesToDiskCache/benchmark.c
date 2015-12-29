#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <string.h>
#define SIZE 256
#define BYTESPERMB 1048576
#define BENCHMARKFILESIZE 10000000 // 10,000,000

//Globals
static struct timeval begin, end;

//Results struct; units are in seconds
struct result_ratios {
  double readdisk;
  double readcache;
  double writedisk;
  double writecache;
} results;

double sincestart(){
  return ((double)end.tv_sec + end.tv_usec/1000000.0) 
       - ((double)begin.tv_sec + begin.tv_usec/1000000.0); 
}
//To measure reading from disk, we simply measure the 'read()' system call.
//It's important we measure this on the first read (so not to measure cache).
double readdisk(int fd){
  ssize_t bytes_read;
  char buf[BENCHMARKFILESIZE];
  gettimeofday(&begin,NULL);                           //start timer
  bytes_read = read(fd, buf, BENCHMARKFILESIZE);       //read
  gettimeofday(&end,NULL);                             //end timer
  if (bytes_read <= 0)
    perror("Read Disk");
  return sincestart();
}
//Reading from cache is measured by reading again what readdisk() already read
//(thus in cache). In main() we use lseek() to ensure we are reading same bytes.
double readcache(int fd){
  ssize_t bytes_read;
  char buf[BENCHMARKFILESIZE];
  gettimeofday(&begin,NULL);                           //start timer
  bytes_read = read(fd, buf, BENCHMARKFILESIZE);       //read
  gettimeofday(&end,NULL);                             //end timer
  if (bytes_read <= 0)
    perror("Read Cache");
  return sincestart();
}
//Writing to disk is measured by first writing (which goes to buffer), then
//performing an fdatasync which syncs cache to disk.
double writedisk(int fd) {
  char buf[BENCHMARKFILESIZE];
  int returnval;
  ssize_t bytes_written;
  gettimeofday(&begin,NULL);                           //start timer
  bytes_written = write(fd, buf, BENCHMARKFILESIZE);   //write
  returnval = fdatasync(fd);                           //fdatasync
  gettimeofday(&end,NULL);                             //end timer
  if (bytes_written <= 0)
    perror("Write Disk");
  if(returnval == -1)
    perror("Write Disk (fdatasync)");
  return sincestart();
}
//Writing to cache is measured by issuing a write command
double writecache(int fd) {
  char buf[BENCHMARKFILESIZE];
  int randominput;
  ssize_t bytes_read, bytes_written;

  //first fill up buffer 'buf' with some data to write out
  randominput = open("/dev/urandom", O_RDONLY);
  if (randominput == -1)
    perror("Opening urandom");
  bytes_read = read(randominput, buf, BENCHMARKFILESIZE);
  if (bytes_read <= 0)
    perror("Reading urandom");
  close(randominput);

  gettimeofday(&begin,NULL);                           //start timer
  bytes_written = write(fd, buf, BENCHMARKFILESIZE);   //write
  gettimeofday(&end,NULL);                             //end timer
  if (bytes_written <= 0)
    perror("Write Cache");
  return sincestart();
}
void print(){
  long bench_mbs = BENCHMARKFILESIZE / BYTESPERMB;
  fprintf(stdout,"***==> Results <==***\n");
  fprintf(stdout,"Read from disk: %f MB/S\n", bench_mbs / results.readdisk);
  fprintf(stdout,"Read from cache: %f MB/S\n", bench_mbs / results.readcache);
  fprintf(stdout,"Write to disk: %f MB/S\n", bench_mbs / results.writedisk);
  fprintf(stdout,"Write to cache: %f MB/S\n", bench_mbs / results.writecache);
  /* ==> Output for ratios <==
  fprintf(stdout,"***==> Ratios <==*** \n");
  fprintf(stdout, "READ Ratio cache/disk: %f\n",
                  results.readdisk / results.readcache);
  fprintf(stdout, "WRITE Ratio cache/disk: %f\n", 
                  results.writedisk / results.writecache);
  */
}
int main() {
  char filename[SIZE];
  int benchread, benchwrite;
  strcpy(filename,"/comp/111/assignments/a6/testfile.dat");
  benchread = open(filename, O_RDONLY);                //open testfile.dat
  if (benchread == -1)
    perror("Open testfile.dat");

  results.readdisk = readdisk(benchread);              //measure disk read
  if (lseek(benchread, 0, SEEK_SET) == -1)
    perror("lseek");
  results.readcache = readcache(benchread);            //measure cache read

  benchwrite = open("temp.txt", O_WRONLY | O_CREAT, 0777);
  results.writecache = writecache(benchwrite);         //measure cache write
  results.writedisk = writedisk(benchwrite);           //measure disk write

  print();
  close(benchread);
  close(benchwrite);
  remove("temp.txt");
}
