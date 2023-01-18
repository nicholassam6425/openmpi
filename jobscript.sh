#!/bin/bash 
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=2
#SBATCH --time=00:02:00
#SBATCH --job-name mpi_job
#SBATCH --output=mpi_output_%j.txt
#SBATCH --mail-type=FAIL
cd $SLURM_SUBMIT_DIR
module load intel
module load openmpi
mpirun ./a.out
# or "srun ./mpi_example"