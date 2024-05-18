#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

int main(int argc, char** argv){
    int process_Rank, size_Of_Cluster;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);

    //printf("Hello World from process %d of %d\n", process_Rank, size_Of_Cluster);

    char msg[16];
    char recv[2];
    char dest[16];

    if(process_Rank == 0){
        strcpy(msg, "message");
    }

    int out = MPI_Scatter(msg, 2, MPI_CHAR, recv, 2, MPI_CHAR, 0,  MPI_COMM_WORLD);

    printf("Process %d recv %s from rank 0\n", process_Rank, recv);

    MPI_Gather(recv, 2, MPI_CHAR, dest, 2, MPI_CHAR, 0, MPI_COMM_WORLD);

    if(process_Rank == 0){
        printf("Proceess %d recv %s from all other processes\n", process_Rank, dest);
    }

    MPI_Finalize();

    return 0;
}
