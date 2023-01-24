#include <stdio.h>
#include ʺmpi.hʺ
int main(int argc, char **argv) {
int num_procs;
int ID;
if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
printf(" ERROR ");
}
MPI_Comm_size (MPI_COMM_WORLD, &num_procs);
MPI_Comm_rank (MPI_COMM_WORLD, &ID);
printf(" hello world from process %d of %d \n",ID,num_procs);
MPI_Finalize();
}
