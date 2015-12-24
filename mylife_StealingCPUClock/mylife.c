#define _POSIX_C_SOURCE 199309 
#include <time.h>
#include <sys/time.h>

#include <stdio.h> 
#include <string.h> 
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0 
#define BUFSIZE 300

//#define DEBUG 1

//global variables:
struct timespec zero = {0,0}; // sleep for zero nanoseconds=after next tick!
typedef long cycle_t;
int iter_i, iter_j;
int row_num,col_num;          //used for work of determine_neighbors()
                              // and evolve()
void init() {
  row_num=0;
  col_num=0;
}

inline cycle_t get_cycles()
{
    cycle_t ret;
    asm volatile("rdtsc" : "=A" (ret));
    //printf("ret: %ld\n",ret);
    return ret;
}

cycle_t calculate_cycles_per_tick()
{
    int i; 
    cycle_t result;
    nanosleep(&zero,NULL); // sync with tick
    cycle_t start = get_cycles();
    for(i=0 ; i<100 ; i++)
        nanosleep(&zero,NULL);
    result=(get_cycles() - start)/100;
    //To protect against case where get_cycles() wraps around
    // and produces a negative difference
    if (result < 0)
      return calculate_cycles_per_tick();

    return result;
}


/* global variables contain grid data */ 
int **grid; 
int **neighbors; 

/* make a grid of cells for the game of life */ 
int **make_grid(int rows,int cols) { 
  int **out = (int **)malloc (rows*sizeof (int *)); 
  int r; 
  for (r=0; r<rows; r++)
	  out[r] = (int *)malloc (cols * sizeof(int)); 
    return out; 
} 

/* make all cells non-living */ 
void zero_grid(int **cells, int rows, int cols) { 
    int r, c; 
    for (r=0; r<rows; r++) 
	for (c=0; c<cols; c++) 
	    cells[r][c]=0; 
} 

/* read a grid of cells from a text file */ 
int **read_grid(FILE *f, int *rows, int *cols) { 
   char buffer[BUFSIZE]; 
    fgets(buffer, BUFSIZE, f); 
    while (buffer[0]=='#') fgets(buffer, BUFSIZE, f); 
    if (sscanf(buffer,"x = %d, y = %d",cols,rows)==2) { 
	int **grid = make_grid(*rows, *cols); 
	int r=0; 
	zero_grid(grid, *rows, *cols); 
	while (! feof(f) && r<(*rows)) { 
	    int c; 
	    fgets(buffer, BUFSIZE, f); 
	    for (c=0; c<BUFSIZE && c<(*cols) 
              && buffer[c] != '\n' && buffer[c] != '\0'; ++c) { 
		if (buffer[c]=='.' || buffer[c]==' ') { 
		    grid[r][c]=0; 
		} else { 
		    grid[r][c]=1; 
		} 
	    } 
	    ++r; 
	} 
	return grid; 
    } else { 
	fprintf(stderr, "first line does not contain grid dimensions\n"); 
	return NULL; 
    } 
} 

/* print a grid in a form that can be read back in */ 
void print_grid(FILE *fp, int **g,int rows,int cols) { 
    int r, c; 
    fprintf(fp, "x = %d, y = %d\n", cols, rows); 
    for (r=0; r<rows; r++) {
	for (c=0; c<cols; c++) { 
	    fprintf(fp,(g[r][c]?"*":".")); 
	}
	printf("\n"); 
    } 
} 

void free_grid(int **grid, int rows) { 
    int i; 
    for (i=0; i<rows; i++) 
        free(grid[i]); 
    free(grid); 
} 

int **grid = NULL; 
int **neighbors = NULL; 

int determine_neighbors(int **cells, int rows, int cols){
  //In order to not do two nested for loops, we'll make this linear
  if (row_num == rows && col_num == 0) return 0;

  int n = 0; 
  if (col_num>0 && cells[row_num][col_num-1]) n++; 
  if (row_num>0 && col_num>0 && cells[row_num-1][col_num-1]) n++; 
  if (row_num>0 && cells[row_num-1][col_num]) n++; 
  if (row_num>0 && col_num<cols-1 && cells[row_num-1][col_num+1]) n++; 
  if (col_num<cols-1 && cells[row_num][col_num+1]) n++; 
  if (row_num<rows-1 && col_num<cols-1 && cells[row_num+1][col_num+1]) n++; 
  if (row_num<rows-1 && cells[row_num+1][col_num]) n++; 
  if (row_num<rows-1 && col_num>0 && cells[row_num+1][col_num-1]) n++; 
  neighbors[row_num][col_num]=n; 
  
  if (++col_num == cols) { col_num=0; ++row_num; }  
  return 1;
}

int evolve(int **cells, int rows, int cols) {
  if (row_num == rows && col_num == 0) return 0;

  if (cells[row_num][col_num] && neighbors[row_num][col_num]<2 
                              || neighbors[row_num][col_num]>3) 
     cells[row_num][col_num] = FALSE; 
        /* any dead cell with three neighbors lives */ 
  else if (!cells[row_num][col_num] && neighbors[row_num][col_num]==3) 
     cells[row_num][col_num] = TRUE; 

  if (++col_num == cols) { col_num=0; ++row_num; }  
  return 1;
}

void next(int **cells, int rows, int cols, int total_iterations) { 
  //we hard code a fraction of .9 for percetage of tick we dare to work
  //testing showed that .9 performed better than BOTH .8 and .98
  double sneak_fraction = .9;
  //determine how many cycles are in a single CPU tick
  cycle_t cycles_per_tick = calculate_cycles_per_tick();
  //determine how many cycles we are going to allow for our sneak
  cycle_t cycles_per_sneak = (sneak_fraction) * cycles_per_tick;
  //Calibrated cycles per cell calculation for neighbors (n) and evolve (e)
  cycle_t cycles_per_cell_n, cycles_per_cell_e;
  //Will calculate cells allowed per tick for neighbors (n) and evolve (e)
  cycle_t cells_allowed_per_tick_n, cells_allowed_per_tick_e;
  cycle_t current_cells_allowed;
  //Allow cycles to carry over from (n) to (e) and back to (n), so forth
  cycle_t cycles_carry_over = 0;
  cycle_t tick_start, cycles_since_tick;
  int iteration, while_alive=1, count = 0;

  // *** START ITERATIONS ***
  for (iteration=0; iteration < total_iterations; iteration++){
    //DETERMINE NEIGHBOR
    init();
    if (iteration == 0) {                //fist iteration, let's calibrate
      tick_start = get_cycles();
      while (while_alive){
        count++;
        if(!determine_neighbors(cells, rows, cols)) while_alive=0;
      }
      while_alive=1;
      cycles_since_tick = get_cycles() - tick_start;
      cycles_per_cell_n = (cycles_since_tick) / (count); 
      cells_allowed_per_tick_n = (cycles_per_sneak) / (cycles_per_cell_n);
    }
    else {
      while (while_alive){
        //need to check if count (cells processed) is greater than cells
        //allowed. We've already spent some cycles, however, in the
        //previous grid (cycles_carry_over). Thus lets calculate how many of
        //this type of cell that is by dividing carry over by
        //how long to process each cell; this let's us know how many cells
        //we have left before expiring. Does not account for cycles spent
        //on: else/while/if/etc
        current_cells_allowed = cells_allowed_per_tick_n 
                              - ((cycles_carry_over)/(cycles_per_cell_n));
        for(; (count < current_cells_allowed)
                  && (while_alive); count++)
          if(!determine_neighbors(cells, rows, cols)) while_alive=0;

          //if break out of for loop and still in grid (alive), then must sleep
          if (while_alive) {
            nanosleep(&zero,NULL);
            cycles_carry_over = 0;
            count = 0;
          }
      } // when exit here, done processing neighbors grid (all cells)
        // presumably count is not zero, so we have a guess how many cycles
        // we are into the current tick by muliplying count (cells processed)
        // by cycles_per_cell_n;
    cycles_carry_over += count * cycles_per_cell_n;
    while_alive=1;
    }

    //EVOLVE
    init();
    count = 0;
    if (iteration == 0) {
      tick_start = get_cycles();
      while (while_alive){
        count++;
        if(!evolve(cells, rows, cols)) while_alive=0;
      }
      while_alive=1;
      cycles_since_tick = get_cycles() - tick_start;
      cycles_per_cell_e = (cycles_since_tick) / (count); 
      cells_allowed_per_tick_e = (cycles_per_sneak) / (cycles_per_cell_e);
      nanosleep(&zero,NULL);
    }
    else {
      while (while_alive){
        current_cells_allowed = cells_allowed_per_tick_e 
                              - ((cycles_carry_over)/(cycles_per_cell_e));
        for(; (count < current_cells_allowed)
                  && (while_alive); count++)
          if(!evolve(cells, rows, cols)) while_alive=0;
          if (while_alive) {
            nanosleep(&zero,NULL);
            cycles_carry_over = 0;
            count = 0;
          }
      } 
    cycles_carry_over += count * cycles_per_cell_e;
    while_alive=1;
    }

#ifdef DEBUG //from for loop that's in main
    printf("next\n"); print_grid(stdout,grid,rows,cols);
#endif //DEBUG
  }
}

/* read an image and generate a result */ 
int main(int argc, char **argv) {
   int rows=0; int cols=0; int iterations=0; int i; 
   if (argc<2 || !(grid=read_grid(stdin,&rows,&cols)) 
     || (iterations=atoi(argv[1]))<0) {
	fprintf(stderr,"life usage:  life iterations <inputfile\n");
	exit(1);
   }
#ifdef DEBUG
   printf("input:\n"); print_grid(stdout,grid,rows,cols);
#endif /* DEBUG */ 
  neighbors = make_grid(rows,cols); 
	next(grid, rows, cols, iterations);
  print_grid(stdout,grid,rows,cols);
  free_grid(grid,rows);
  free_grid(neighbors,rows);
} 
