#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdbool.h>
#include<string.h>
#include<assert.h>

#include <mpi.h>

#define ROOT 0
#define PADDING 1

typedef unsigned char BYTE;

// full size base world to be scattered
BYTE *g_data=NULL;

// mini MPI-worlds
BYTE* mini_world;
BYTE* mini_world_result;

// Current width of world.
size_t g_worldWidth=0;

/// Current height of world.
size_t g_worldHeight=0;

/// Current data length (product of width and height)
size_t g_dataLength=0;  // g_worldWidth * g_worldHeight

// MPI, mini world info
int RANK, CLUSTER_SIZE;
size_t mini_width, mini_height, mini_size;

// representational info
size_t rows_per_rank, cells_per_rank;

#ifdef DEBUG
int DEBUG = 1;
#endif

#ifndef DEBUG
int DEBUG = 0;
#endif

static inline void HL_initAllZeros( size_t worldWidth, size_t worldHeight )
{
    g_worldWidth = worldWidth;
    g_worldHeight = worldHeight;
    g_dataLength = g_worldWidth * g_worldHeight;

    // calloc init's to all zeros
    g_data = calloc( g_dataLength, sizeof(unsigned char));
}

static inline void HL_initAllOnes( size_t worldWidth, size_t worldHeight )
{
    int i;
    
    g_worldWidth = worldWidth;
    g_worldHeight = worldHeight;
    g_dataLength = g_worldWidth * g_worldHeight;

    g_data = calloc( g_dataLength, sizeof(unsigned char));

    // set all rows of world to true
    for( i = 0; i < g_dataLength; i++)
    {
	g_data[i] = 1;
    }
    
}

static inline void HL_initOnesInMiddle( size_t worldWidth, size_t worldHeight )
{
    int i;
    
    g_worldWidth = worldWidth;
    g_worldHeight = worldHeight;
    g_dataLength = g_worldWidth * g_worldHeight;

    g_data = calloc( g_dataLength, sizeof(unsigned char));

    // set first 1 rows of world to true
    for( i = 10*g_worldWidth; i < 11*g_worldWidth; i++)
    {
	if( (i >= ( 10*g_worldWidth + 10)) && (i < (10*g_worldWidth + 20)))
	{
	    g_data[i] = 1;
	}
    }
    
}

static inline void HL_initOnesAtCorners( size_t worldWidth, size_t worldHeight )
{
    g_worldWidth = worldWidth;
    g_worldHeight = worldHeight;
    g_dataLength = g_worldWidth * g_worldHeight;

    g_data = calloc( g_dataLength, sizeof(unsigned char));

    g_data[0] = 1; // upper left
    g_data[worldWidth-1]=1; // upper right
    g_data[(worldHeight * (worldWidth-1))]=1; // lower left
    g_data[(worldHeight * (worldWidth-1)) + worldWidth-1]=1; // lower right
    
}

static inline void HL_initSpinnerAtCorner( size_t worldWidth, size_t worldHeight )
{
    g_worldWidth = worldWidth;
    g_worldHeight = worldHeight;
    g_dataLength = g_worldWidth * g_worldHeight;

    g_data = calloc( g_dataLength, sizeof(unsigned char));

    g_data[0] = 1; // upper left
    g_data[1] = 1; // upper left +1
    g_data[worldWidth-1]=1; // upper right
    
}

static inline void HL_initReplicator( size_t worldWidth, size_t worldHeight )
{
    size_t x, y;
    
    g_worldWidth = worldWidth;
    g_worldHeight = worldHeight;
    g_dataLength = g_worldWidth * g_worldHeight;

    g_data = calloc( g_dataLength, sizeof(unsigned char));

    x = worldWidth/2;
    y = worldHeight/2;
    
    g_data[x + y*worldWidth + 1] = 1; 
    g_data[x + y*worldWidth + 2] = 1;
    g_data[x + y*worldWidth + 3] = 1;
    g_data[x + (y+1)*worldWidth] = 1;
    g_data[x + (y+2)*worldWidth] = 1;
    g_data[x + (y+3)*worldWidth] = 1; 
    
}

static inline void HL_initMaster( unsigned int pattern, size_t worldWidth, size_t worldHeight )
{
    switch(pattern)
    {
    case 0:
	HL_initAllZeros( worldWidth, worldHeight );
	break;
	
    case 1:
	HL_initAllOnes( worldWidth, worldHeight );
	break;
	
    case 2:
	HL_initOnesInMiddle( worldWidth, worldHeight );
	break;
	
    case 3:
	HL_initOnesAtCorners( worldWidth, worldHeight );
	break;

    case 4:
	HL_initSpinnerAtCorner( worldWidth, worldHeight );
	break;

    case 5:
	HL_initReplicator( worldWidth, worldHeight );
	break;
	
    default:
	printf("Pattern %u has not been implemented \n", pattern);
	exit(EXIT_FAILURE);
    }
}

static inline void HL_swap( unsigned char **pA, unsigned char **pB)
{
  unsigned char *temp = *pA;
  *pA = *pB;
  *pB = temp;
}
 
static inline unsigned int HL_countAliveCells(unsigned char* data, 
					   size_t x0, 
					   size_t x1, 
					   size_t x2, 
					   size_t y0, 
					   size_t y1,
					   size_t y2) 
{
  
  return data[x0 + y0] + data[x1 + y0] + data[x2 + y0]
    + data[x0 + y1] + data[x2 + y1]
    + data[x0 + y2] + data[x1 + y2] + data[x2 + y2];
}

// Don't Modify this function or your submitty autograding will not work
static inline void print_world()
{
    for(int i=0;i<g_worldHeight;++i){
        for(int j=0;j<g_worldWidth;++j){
            if(g_data[i*g_worldWidth + j] == '\0'){
                printf(".");
            }
            else{
                printf("#");
            }
        }
        printf("\n");
    }
}

static inline void print_chunk(){
    for(int i=0;i<mini_height;++i){
        for(int j=0;j<mini_width;++j){
            if(mini_world[i*mini_width+j] == '\0'){
                if(i==0 || i==mini_height-PADDING){
                    printf("-");
                }else{
                    printf(".");
                }
            }else{
                printf("#");
            }
        }
        printf("\n");
    }
}

// Don't Modify this function or your submitty autograding will not work
static inline void HL_printWorld(size_t iteration)
{
    int i, j;

    printf("Print World - Iteration %lu \n", iteration);
    
    for( i = 0; i < g_worldHeight; i++)
    {
	printf("Row %2d: ", i);
	for( j = 0; j < g_worldWidth; j++)
	{
	    printf("%u ", (unsigned int)g_data[(i*g_worldWidth) + j]);
	}
	printf("\n");
    }

    printf("\n\n");
}

/// Serial version of standard byte-per-cell life.
bool HL_iterateSerial() {
// this modified version calculates cell results in padding rows as well...
// which isn't necessary but isn't problematic either
  size_t y, x;

  for (y = 0; y < mini_height; ++y) 
    {
      size_t y0 = ((y + mini_height - 1) % mini_height) * mini_width;
      size_t y1 = y * mini_width;
      size_t y2 = ((y + 1) % mini_height) * mini_width;
      
    for (x = 0; x < mini_width; ++x) 
      {
        size_t x0 = (x + mini_width - 1) % mini_width;
        size_t x2 = (x + 1) % mini_width;
      
        unsigned int aliveCells = HL_countAliveCells(mini_world, x0, x, x2, y0, y1, y2);
        // rule B36/S23
        mini_world_result[x + y1] = (aliveCells == 3) || (aliveCells == 6 && !mini_world[x + y1])
          || (aliveCells == 2 && mini_world[x + y1]) ? 1 : 0;
    }
      }
  HL_swap(&mini_world, &mini_world_result);

  // HL_printWorld(i);
  
  return true;
}

int modulo(int a, int b){
    int val = a % b;
    if (val >= 0){return val;}
    else{
        return b + a;
    }
}

int main(int argc, char *argv[]){

    if( argc != 4 ){
        printf("Usage: ./a.out <pattern> <worldSize> <iterations>\n");
        exit(EXIT_FAILURE);
    }

    // set up launch args
    
    size_t pattern = atoi(argv[1]);
    size_t worldSize = atoi(argv[2]);
    size_t iterations = atoi(argv[3]);
    
    g_worldWidth = worldSize;
    g_worldHeight = worldSize;
    g_dataLength = worldSize * worldSize;

    // set up MPI

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &CLUSTER_SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);

    if(g_worldHeight % CLUSTER_SIZE != 0){
        printf("Cluster size %d does not evenly divide world height %zu - exiting\n", CLUSTER_SIZE, g_worldHeight);
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    // set up multiprocessing info
    
    rows_per_rank = g_worldHeight / CLUSTER_SIZE;
    mini_height = rows_per_rank + 2*PADDING;
    mini_width = g_worldWidth;
    mini_size = mini_height * mini_width;

    cells_per_rank = rows_per_rank * g_worldWidth;
    size_t padding_size = PADDING * mini_width;
    assert(cells_per_rank == g_dataLength / CLUSTER_SIZE);


    // set up world
    
    if(RANK == ROOT){
        // rank 0 process has responsibility of setting up initial world
        HL_initMaster(pattern, g_worldWidth, g_worldHeight);

        printf("\nRUNNING SIM WITH CLUSTER SIZE: %d FOR %zu ITERATIONS\n", CLUSTER_SIZE, iterations);
        printf("Base world dim: %zu\n", g_worldWidth);
        printf("Chunk height (with padding): %zu\n", mini_height);
        double wtime = MPI_Wtime();

    }

    // all ranks set up mini worlds...
    mini_world = calloc(mini_size, sizeof(BYTE));
    mini_world_result = calloc(mini_size, sizeof(BYTE));

    // scatter world
    MPI_Scatter(g_data, cells_per_rank, MPI_CHAR, mini_world+padding_size, cells_per_rank, MPI_CHAR, ROOT, MPI_COMM_WORLD);
    if(RANK == ROOT){
        free(g_data);
    }

    // release
    for(int iter = 0;iter<iterations;++iter){
        // each rank should now have their requisite data to start processing cells
        
        // iterate
        // each iteration contains:
        // - synchronize (barrier)
        // - share padding info
        //   - this only needs to happen with the prev world, not result world
        // - compute worlds
        // - swap worlds
        
        // padding sharing - even numbered ranks share first

        // send row info, recv into padding
        BYTE* upper_padding = mini_world;
        BYTE* lower_padding = mini_world + padding_size + cells_per_rank;
        BYTE* upper_row = mini_world + padding_size;
        BYTE* lower_row = mini_world + cells_per_rank;

        if(RANK%2 == 0){
            // send first, send +1 first then -1
            // find target rank for +1 send

            if(RANK == ROOT){
                printf("Solving iteration %d\n", iter);
            }

            int TARGET_RANK = modulo(RANK + 1, CLUSTER_SIZE);
            MPI_Send(lower_row, padding_size, MPI_CHAR, TARGET_RANK, 0, MPI_COMM_WORLD);

            TARGET_RANK = modulo(RANK - 1, CLUSTER_SIZE);
            MPI_Send(upper_row, padding_size, MPI_CHAR, TARGET_RANK, 0, MPI_COMM_WORLD);

            // recv next
            int SOURCE_RANK = modulo(RANK - 1, CLUSTER_SIZE);
            MPI_Recv(upper_padding, padding_size, MPI_CHAR, SOURCE_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            SOURCE_RANK = modulo(RANK + 1, CLUSTER_SIZE);
            MPI_Recv(lower_padding, padding_size, MPI_CHAR, SOURCE_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        }else{
            // recv first, recv from -1 and then from +1
            int SOURCE_RANK = modulo(RANK - 1, CLUSTER_SIZE);
            MPI_Recv(upper_padding, padding_size, MPI_CHAR, SOURCE_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            SOURCE_RANK = modulo(RANK + 1, CLUSTER_SIZE);
            MPI_Recv(lower_padding, padding_size, MPI_CHAR, SOURCE_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
            // send next
            int TARGET_RANK = modulo(RANK + 1, CLUSTER_SIZE);
            MPI_Send(lower_row, padding_size, MPI_CHAR, TARGET_RANK, 0, MPI_COMM_WORLD);

            TARGET_RANK = modulo(RANK - 1, CLUSTER_SIZE);
            MPI_Send(upper_row, padding_size, MPI_CHAR, TARGET_RANK, 0, MPI_COMM_WORLD);
        
        }

        HL_iterateSerial();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // gather chunks back into world
    if(RANK == ROOT){
        g_data = calloc(g_dataLength, sizeof(BYTE));
    }
    MPI_Gather(mini_world+padding_size, cells_per_rank, MPI_CHAR, g_data, cells_per_rank, MPI_CHAR, ROOT, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    if(RANK == ROOT){
        printf("MPI CPU TIME: %f\n\n", MPI_Wtime());
        free(g_data);
    }
    
    if(RANK == ROOT && DEBUG){
        printf("FINAL WORLD:\n");
        print_world();
    }

    free(mini_world);
    free(mini_world_result);


    MPI_Finalize();
    
    return EXIT_SUCCESS;
}
