/***************************************************************************/
/* Template for Asssignment 1 **********************************************/
/* Your Name Here             **********************************************/
/***************************************************************************/

/***************************************************************************/
/* Includes ****************************************************************/
/***************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<math.h>

/***************************************************************************/
/* Defines *****************************************************************/
/***************************************************************************/

#define MAX_CORE_DOUBLINGS 20
#define MAX_NODES 1<< (MAX_CORE_DOUBLINGS/2) // 2^10
#define MAX_CORES_PER_NODE 1<< (MAX_CORE_DOUBLINGS/2) // 2^10
#define MAX_CORES_IN_SYSTEM (MAX_NODES * MAX_CORES_PER_NODE)
#define MAX_F 5
#define PERF(r) sqrt((double)r)

/***************************************************************************/
/* Global Vars *************************************************************/
/***************************************************************************/

double f[MAX_F] = {0.99999999, 0.9999, 0.99, 0.9, 0.5};

/***************************************************************************/
/* Function Decs ***********************************************************/
/***************************************************************************/

void compute_speedups();

/***************************************************************************/
/* Function: Main **********************************************************/
/***************************************************************************/

int main(int argc, char** argv){

  if(argc!=2){
    printf("Usage: ./multicore.out <f>\n");
    exit(1);
  }

  int selected_f = atoi(argv[1]);

  compute_speedups(selected_f);
  return 0;
}

/***************************************************************************/
/* Function: compute_speedups **********************************************/
/***************************************************************************/

double compute_speedup_asym(double N, double f, double c, double r){
  return N*1/((1-f)/PERF(r) + f/(PERF(r) + c - r));
}

double compute_speedup_dyn(double N, double f, double c, double r){
  return N*1/((1-f)/PERF(r) + f/c);
}

void compute_speedups(int selected_f)
{
  int f_index=0, r_index=0, core_index=0, node_index=0;
  double speedup_asymmetric = 0.0;
  double speedup_dynamic = 0.0;

  // dummy assignments to remove compiler warning on unused variables
  f_index = r_index;
  core_index = f_index;
  node_index= core_index;
  r_index = node_index;
  
  speedup_asymmetric = speedup_dynamic;
  speedup_dynamic = speedup_asymmetric;
  
  // INSERT YOUR CODE HERE
  // Hint: you will have 4 loops
  //       First loop: f_index: 0 to MAX_F by increments of 1
  //       2nd loop: node_index: 1 to MAX_NODES by increments of 4x
  //          Hint: to mult a variable by 4x just use left shift by 2 operator e.g., node_index = node_index << 2
  //       3nd loop: r_index: 1 to MAX_CORES_PER_NODE by increments of 4x
  //       4rd loop: core_index: r_index to MAX_CORES_PER_NODE by increments 4x
  //       Compute both speedup_asymmetric and speedup_dynamic and output results

  for(int f_index = 0;f_index<MAX_F;++f_index){
    // open new csv file for data
    char filename[16];
    sprintf(filename, "DATA_%d.csv", f_index);
    FILE* csvfile = fopen(filename, "w");

    if (csvfile == NULL) {
        printf("Error opening file!");
        exit(1); // Return an error code
    }

    fprintf(csvfile, "total_cores,r_index,speedup_asym,speedup_dyn\n");

    for(node_index=1;node_index<(MAX_NODES+1);node_index = node_index<<2){
      
      for(r_index=1;r_index<MAX_CORES_PER_NODE+1;r_index = r_index<<2){

        for(core_index=r_index;core_index<MAX_CORES_PER_NODE+1;core_index = core_index<<2){

          double total_cores = node_index*core_index;
          speedup_asymmetric = compute_speedup_asym(node_index, f[f_index], core_index, r_index);
          speedup_dynamic = compute_speedup_dyn(node_index, f[f_index], core_index, r_index);

          if(f_index == selected_f){
            printf("%10.8lf %8u %6u %6u %6u %12.4lf %12.4lf \n", f[f_index], (node_index * core_index), node_index, r_index, core_index, speedup_asymmetric, speedup_dynamic);
          }
          fprintf(csvfile, "%f,%d,%f,%f\n", total_cores, r_index, speedup_asymmetric, speedup_dynamic);

        }
      }
    }

    fclose(csvfile);
    
  }

  return;
}


