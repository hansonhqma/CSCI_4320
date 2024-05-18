#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <cuda.h>
#include <cuda_runtime.h>


#define MAX_GRID_DIM 65535
#define SPAWN_LINE_SIZE 10

typedef unsigned char BYTE;


BYTE* world_prev = NULL;
BYTE* world_result = NULL;

size_t world_width, world_height, world_length;

size_t block_dim;
size_t grid_dim;

size_t increments = 1;

#ifdef SHOW_WORLD
int show_world = 1;
#endif

#ifndef SHOW_WORLD
int show_world = 0;
#endif


#ifdef DEBUG
int debug = 1;
#endif

#ifndef DEBUG
int debug = 0;
#endif


// WORLD IS ALL ZEROS
__global__ void init_allzeros(BYTE* wprev, BYTE* wresult, size_t width, size_t height, int _increments){

    size_t jump = gridDim.x * blockDim.x;
    size_t idx;
    for(int i=0;i<_increments;++i){
        idx = blockIdx.x * blockDim.x + threadIdx.x + jump*i;
        if( idx < width * height){
            // check if idx in bounds
            wprev[idx] = 0;
            wresult[idx] = 0;
        }
    }
    

}

// WORLD IS ALL ONES
__global__ void init_allones(BYTE* wprev, BYTE* wresult, size_t width, size_t height, int _increments){

    size_t jump = gridDim.x * blockDim.x;
    size_t idx;
    for(int i=0;i<_increments;++i){
        idx = blockIdx.x * blockDim.x + threadIdx.x + jump*i;
        if( idx < width * height){
            // check if idx in bounds
            wprev[idx] = 1;
            wresult[idx] = 0;
        }
    }

    //__syncthreads();
}

// STREAK OF 10 ONES IN ABOUT THE MIDDLE OF THE WORLD
__global__ void pattern_two(BYTE* wprev, BYTE* wresult, size_t  width, size_t height, int cell_count){

    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    size_t centerpos = width*height/2;

    if(idx < cell_count){
        idx += centerpos - cell_count / 2;
        wprev[idx] = 1;
    }
}

// ONES AT THE CORNERS OF THE WORLD
__global__ void pattern_three(BYTE* wprev, BYTE* wresult, size_t width, size_t height, int _increments){
    // all we need is 4 threads...?
}


// SPINNER PATTERN AT CORNERS OF WORLD
__global__ void pattern_four(BYTE* wprev, BYTE* wresult, size_t width, size_t height, int _increments){
    
}


// REPLICATOR PATTERN STARTING IN THE MIDDLE
__global__ void pattern_five(BYTE* wprev, BYTE* wresult, size_t width, size_t height){

    // only need 6 threads
    size_t x = width/2;
    size_t y = height/2;

    // this is dumb but im lazy and its just as fast

    switch(threadIdx.x)
    {
        case 0:
            wprev[x + y*width + 1] = 1;
        break;
        case 1:
            wprev[x + y*width + 2] = 1;
        break;
        case 2:
            wprev[x + y*width + 3] = 1;
        break;
        case 3:
            wprev[x+(y+1)*width] = 1;
        break;
        case 4:
            wprev[x+(y+2)*width] = 1;
        break;
        case 5:
            wprev[x+(y+3)*width] = 1;
        break;
    }
    
}

// core game functions
// game_initMaster: allocates memory and initializes worlds based on pattern
// game_kernelLaunch: driver function to start game
// __global__ game_kernelRun: launches CUDA kernel and runs one iteration of world
// game_swap: swaps prev and next world pointers (device memory)

// cant really increment within thread... since we need synchronization


void game_initMaster(int pattern){
    // allocates device memory for world, calls world init functions

    if(debug){
        printf("creating worlds of size %d x %d\n", world_width, world_height);
    }

    cudaError_t out = cudaMalloc((void**)&world_prev, world_length);
    if(out){
        fprintf(stderr, "cudaMalloc failed, error code %d\n", out);
    }

    out = cudaMalloc((void**)&world_result, world_length);
    if(out){
        fprintf(stderr, "cudaMalloc failed, error code %d\n", out);
    }

    switch(pattern){
        case 0:
            init_allzeros<<<grid_dim, block_dim>>>(world_prev, world_result, world_width, world_height, increments);
        break;

        case 1:
            printf("calling init_allones\n");
            init_allones<<<grid_dim, block_dim>>>(world_prev, world_result, world_width, world_height, increments);
        break;
        
        case 2:
            init_allzeros<<<grid_dim, block_dim>>>(world_prev, world_result, world_width, world_height, increments);
            cudaDeviceSynchronize();
            // dont need to launch shit ton of threads for spawning the line, only 10
            pattern_two<<<1, SPAWN_LINE_SIZE>>>(world_prev, world_result, world_width, world_height, SPAWN_LINE_SIZE);
        break;
            
        case 3:
            init_allzeros<<<grid_dim, block_dim>>>(world_prev, world_result, world_width, world_height, increments);

        break;

        case 4:
            init_allzeros<<<grid_dim, block_dim>>>(world_prev, world_result, world_width, world_height, increments);

        break;

        case 5:
            init_allzeros<<<grid_dim, block_dim>>>(world_prev, world_result, world_width, world_height, increments);
            // 6 threads for replicator pattern
            pattern_five<<<1, 6>>>(world_prev, world_result, world_width, world_height);
        break;
    }

}

__device__ unsigned int check_cell(BYTE* world, int x, int y, size_t width, size_t height){
    if(x < 0 || x >= width || y < 0 || y >= height){
        return 0;
    }
    else{
        size_t idx = y * width + x;
        return world[idx];
    }
}

__global__ void kernel_run(BYTE* wprev, BYTE* wresult, size_t width, size_t height, int _increments){

    size_t jump = gridDim.x * blockDim.x;

    for(int i=0;i<_increments;++i){
        // compute where in array we need to update
        size_t idx = blockIdx.x * blockDim.x + threadIdx.x + jump * i;
        if(idx < width*height){
            size_t x = idx % width;
            size_t y = idx / width;
            int alive = check_cell(wprev, x-1, y, width, height) +
                        check_cell(wprev, x+1, y-1, width, height) +
                        check_cell(wprev, x, y-1, width, height) +
                        check_cell(wprev, x-1, y-1, width, height) +
                        check_cell(wprev, x+1, y, width, height) +
                        check_cell(wprev, x-1, y+1, width, height) +
                        check_cell(wprev, x, y+1, width, height) +
                        check_cell(wprev, x+1, y+1, width, height);
            wresult[idx] = (alive == 3) || (alive == 6 && !wprev[idx]) || (alive == 2 && wprev[idx]) ? 1 : 0;
        }
    }
}

static inline void swap_worlds(BYTE **A, BYTE **B){
    BYTE* temp = *A;
    *A = *B;
    *B = temp;
}

void print_world(BYTE* world, size_t dim){

    
    if(dim > 64){
        fprintf(stderr, "cannot print world larger than dim 64\n");
    }
    else{
        // have to copy device mem back to host mem
        BYTE* hostworld = (BYTE*)calloc(dim*dim, sizeof(BYTE));
        cudaError_t out = cudaMemcpy(hostworld, world, dim*dim, cudaMemcpyDeviceToHost);
        if(out){
            fprintf(stderr, "cudaMemcpy failed with %d\n", out);
        }
        for(int i=0;i<dim;++i){
            for(int j=0;j<dim;++j){
                int idx = i*dim + j;
                if(hostworld[idx]){
                    printf("#");
                }else{
                    printf("-");
                }
            }
            printf("\n");
        }
        free(hostworld);
    }

}

void kernel_launch(int iterations){
    // assume world is already constructed

    for(int i=0;i<iterations;++i){
        if(debug){
            printf("Running iteration %d\n", i);
        }
        kernel_run<<<grid_dim, block_dim>>>(world_prev, world_result, world_width, world_height, increments);

        cudaError_t out = cudaDeviceSynchronize();
        if(out){
           fprintf(stderr, "cudaDeviceSynchronize failed with code %d\n", out);
        }

        if(show_world){
            print_world(world_result, world_height);
        }

        swap_worlds(&world_prev, &world_result);
        
    }
    
    printf("-- RUNNING %d KERNEL ITERATIONS WITH: --\n", iterations);
    printf("World dim: %d x %d\n", world_width, world_height);
    printf("World length: %zu\n", world_length);
    printf("Increments: %d\n\n", increments);
    printf("Blocks: %d\n", grid_dim);
    printf("Threads per block: %d\n",  block_dim);
    printf("Cells reachable: %d\n", grid_dim * block_dim);
    printf("Estimated worlds memory usage: %d MiB\n\n", world_length>>(19));
}

inline static void game_free(){
    cudaError_t out = cudaFree(world_prev);
    if(out){
        fprintf(stderr, "cudaFree failed with code %d\n", out);
    }

    out = cudaFree(world_result);
    if(out){
        fprintf(stderr, "cudaFree failed with code %d\n", out);
    }
}

size_t count_total_alive_cells(BYTE* world){
    printf("counting world cells\n");
    size_t total = 0;
    for(size_t i=0;i<world_length;++i){
        if(world[i]){
            total++;
        }
    }
    return total;
}



int main(int argc, char** argv){


    if(argc!=5){
        printf("usage: ./hl.out <pattern> <world_dim> <iter> <block_dim>\n");
        exit(EXIT_FAILURE);
    }

    int pattern = atoi(argv[1]);
    int world_dim = atoi(argv[2]);
    int iterations = atoi(argv[3]);
    block_dim = atoi(argv[4]);

    world_width = world_dim;
    world_height = world_dim;
    world_length = world_width * world_height;


    // we need to figure out exactly how many blocks we need... starting with 1
    // increment blocks to 65535 - then start increasing repetitions

    double blocks_req = ((double)world_length) / ((double)block_dim);

    if( blocks_req < MAX_GRID_DIM ){
        grid_dim = ceil(blocks_req);
    }
    else{
        grid_dim = 65535;
        // calculate increments
        size_t reachable = grid_dim * block_dim;
        increments = ceil( ((double)world_length) / ((double)reachable));
    }

    game_initMaster(pattern);

    cudaError_t out;
    BYTE* hostworld = NULL;

    if(debug){

        hostworld = (BYTE*)calloc(world_length, sizeof(BYTE));
        if(!hostworld){
            fprintf(stderr, "hostworld alloc failed\n");
        }

        out = cudaMemcpy(hostworld, world_prev, world_length, cudaMemcpyDeviceToHost);
        if(out){
            fprintf(stderr, "cudaMemcpy failed with code %d\n", out);
        }

        printf("world starting with %zu alive cells\n", count_total_alive_cells(hostworld));
    }
    

    out = cudaDeviceSynchronize();
    if(out){
        fprintf(stderr, "cudaDevicSynchronize failed with code %d\n", out);
    }

    kernel_launch(iterations);

    if(debug){
        out = cudaMemcpy(hostworld, world_prev, world_length, cudaMemcpyDeviceToHost);
        if(out){
            fprintf(stderr, "cudaMemcpy failed with code %d\n", out);
        }

        printf("world ended with %zu alive cells\n", count_total_alive_cells(hostworld));
        free(hostworld);
    }

    game_free();

    return EXIT_SUCCESS;
}
