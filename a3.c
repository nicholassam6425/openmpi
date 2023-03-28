#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include "mpi.h"

bool isNumber(char number[])
{
    int i = 0;

    if (number[0] == '-')
        return false;
    for (; number[i] != 0; i++)
    {
        if (!isdigit(number[i]))
            return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 2 && isNumber(argv[1]))
    {
        printf("Usage: %s N\nN must be a positive integer greater than 0", argv[0]);
        exit(1);
    }
    int my_rank, num_procs;
    double start_time, end_time, time_elapsed;
    MPI_Status status;
    int *in = NULL;
    int insize;
    int *out = NULL;
    int outsize = 0;
    int N = strtol(argv[1], NULL, 10);
    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // each processor finds which rows to calculate
    int *calc_rows;
    int num_rows = (N/2) / num_procs;
    num_rows *= 2;
    if (my_rank < (N/2) % num_procs)
    {
        num_rows += 2;
    }
    calc_rows = malloc(num_rows * sizeof(int));
    int counter = 0;
    for (int i = 0; counter < num_rows; i++)
    {
        calc_rows[counter++] = (i * num_procs) + (my_rank + 1);
        calc_rows[counter++] = N - ((i * num_procs) + (my_rank));
    }
    // /*  this code block prints the amount of multiplication operations each processor must complete
    counter = 0;
    for (int i = 0; i < num_rows; i++)
    {
        counter += calc_rows[i];
    }
    printf("%d load: %d\n", my_rank, counter);
    // */
    if (N - my_rank < ((int)sqrt(__INT_MAX__)) + 1) // if N is small enough, we use the more memory-intensive method to save on time
    {
        bool *nums;
        nums = malloc(((N - my_rank) * (N - my_rank) + 1) * sizeof(bool));
        memset(nums, false, (N - my_rank) * (N - my_rank) + 1);
        for (int i = 0; i < num_rows; i++)
        {
            for (int j = 1; j <= calc_rows[i]; j++)
            {
                nums[calc_rows[i] * j] = true;
            }
        }
        for (int i = 0; i < (N - my_rank) * (N - my_rank) + 1; i++)
        {
            if (nums[i] == true)
            {
                out = realloc(out, (outsize + 1) * sizeof(int));
                out[outsize++] = i;
            }
        }
    }
    else // otherwise, use the more memory-friendly method, which takes more time
    {
        // set initial value
        out = realloc(out, (outsize + 1) * sizeof(int));
        out[outsize++] = calc_rows[0];
        bool test = false;
        for (int i = 0; i < num_rows; i++) // for each row
        {
            for (int j = 1; j <= calc_rows[i]; j++) // for each column
            {
                for (int k = 0; k < outsize; k++)
                {
                    if (calc_rows[i] * j == out[k])
                    {
                        test = true;
                        break;
                    }
                }
                if (test == true)
                {
                    continue;
                }
                out = realloc(out, (outsize + 1) * sizeof(int));
                out[outsize++] = calc_rows[i] * j;
            }
        }
    }
    if (my_rank == 0)
    {
        int temp;
        for (int i = 1; i < num_procs; i++)
        {
            MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_INT, &insize);
            in = realloc(in, insize * sizeof(int));
            MPI_Recv(in, insize, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
            temp = outsize;
            outsize += insize;
            out = realloc(out, outsize * sizeof(int));
            memcpy(out + temp, in, insize * sizeof(int));
            printf("Proc %d received Proc %d successfully\n", my_rank, i);
        }
        printf("number of elements received: %d\n", outsize);
        // /*
        //radix sort out array O(n)
        long max = N*N;
        int count[10] = {0};
        int output[outsize+1];
        for (long place = 1; max / place > 0; place *= 10)
        {
            memset(count, 0, sizeof(count));
            for (int i = 0; i < outsize; i++)
            {
                count[(out[i]/place) % 10]++;
            }
            for (int i = 1; i < 10; i++)
            {
                count[i] += count[i-1];
            }
            for (int i = outsize-1; i >= 0; i--)
            {
                output[count[(out[i]/place)%10]-1] = out[i];
                count[(out[i]/place)%10]--;
            }
            for (int i = 0; i < outsize; i++)
            {
                out[i] = output[i];
            }
        }
        //then remove duplicates from sorted array O(n)
        printf("Radix sort completed\n");

        int temp2[outsize];
        int j = 0;
        for (int i = 0; i < outsize - 1; i++)
        {
            if (out[i] != out[i+1])
            {
                temp2[j++] = out[i];
            }
        }
        temp2[j++] = out[outsize - 1];
        
        // */
        printf("\n");
        printf("Final output: %d\n", j);
        free(in);
    }
    else
    {
        MPI_Send(out, outsize, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    free(out);
    MPI_Finalize();
    return 0;
}