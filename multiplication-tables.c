#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include "mpi.h"

#define printload 1

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
    long long *in = NULL;
    long long insize;
    long long *out = NULL;
    long long outsize = 0;
    int N = strtol(argv[1], NULL, 10);
    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);



    // each processor finds which rows to calculate
    // this is O(n/p)
    int *calc_rows;
    int num_rows;
    num_rows = (N / 2) / num_procs;
    num_rows *= 2;
    if (my_rank < (N / 2) % num_procs)
    {
        num_rows += 2;
    }
    if (N % 2 != 0)
    {
        if (my_rank == (N / 2))
        {
            num_rows++;
        }
        else if (my_rank == 0 && (N / 2) + 1 >= num_procs)
        {
            num_rows++;
        }
    }
    calc_rows = malloc(num_rows * sizeof(int));
    long long counter = 0;
    for (int i = 0; counter < num_rows; i++)
    {
        calc_rows[counter++] = (i * num_procs) + (my_rank + 1);
        calc_rows[counter++] = N - ((i * num_procs) + (my_rank));
    }
    if (N % 2 != 0)
    {
        if (my_rank == (N / 2))
        {
            calc_rows[counter++] = (N / 2) + 1;
        }
        else if (my_rank == 0 && (N / 2) + 1 >= num_procs)
        {
            calc_rows[counter++] = (N / 2) + 1;
        }
    }
#ifdef printload // this code block prints the amount of multiplication operations each processor must complete
    counter = 0;
    for (int i = 0; i < num_rows; i++)
    {
        counter += calc_rows[i];
    }
    printf("%d load: %d\n", my_rank, counter);
#endif


    // using a modified hash set, populate the hash set with numbers that appear in the processor's products
    // this is O((n/p)^2)
    // using a list, where if you insert X, you traverse the list for X, takes O((n/p)^2 log(n/p)) i think 
    bool *nums;
    long long max_num = ((long long)N - my_rank) * (N - my_rank);
    nums = malloc((max_num) * sizeof(bool));
    memset(nums, false, (max_num) * sizeof(bool));
    for (int i = 0; i < num_rows; i++)
    {
        for (long long j = 1; j <= calc_rows[i]; j++)
        {
            nums[(calc_rows[i] * j) - 1] = true;
        }
    }
    free(calc_rows);
    for (long long i = 0; i < max_num; i++)
    {
        if (nums[i] == true)
        {
            out = realloc(out, (++outsize) * sizeof(long long));
            out[outsize-1] = i+1;
        }
    }
    free(nums);

    // send it all to master proc for final processing
    // uses a list so its O(n^2 logn), but at this point, n is pretty heavily reduced, so its like, basically, n^2 but not really
    // i didn't use hash set here because im scared of crashing teach with memory leaks
    printf("%d outsize: %lld\n", my_rank, outsize);
    if (my_rank == 0)
    {
        long long temp;
        for (int i = 1; i < num_procs; i++)
        {
            //MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
            //MPI_Get_count(&status, MPI_LONG_LONG, &insize);
            // MPI_Get_count only works up to int max, so we have to send/recv the number manually before sending the array
            MPI_Recv(&insize, 1, MPI_LONG_LONG, i, 1, MPI_COMM_WORLD, NULL);
            in = realloc(in, insize * sizeof(long long));
            MPI_Recv(in, insize, MPI_LONG_LONG, i, 0, MPI_COMM_WORLD, NULL);
            temp = outsize;
            outsize += insize;
            out = realloc(out, outsize * sizeof(long long)); 
            memcpy(out + temp, in, insize * sizeof(long long));
        }
        printf("\nnumber of elements received: %lld\n", outsize);
        long long max = N * N;
        long long count[10] = {0,0,0,0,0,0,0,0,0,0};
        long long output[outsize + 1];
        for (long long place = 1; max / place > 0; place *= 10)
        {
            memset(count, 0, 10 * sizeof(long long));
            for (long long i = 0; i < outsize; i++)
            {
                count[(out[i] / place) % 10]++;
            }
            for (long long i = 1; i < 10; i++)
            {
                count[i] += count[i - 1];
            }
            for (long long i = outsize - 1; i >= 0; i--)
            {
                output[count[(out[i] / place) % 10] - 1] = out[i];
                count[(out[i] / place) % 10]--;
            }
            for (long long i = 0; i < outsize; i++)
            {
                out[i] = output[i];
            }
        }
        // then remove duplicates from sorted array O(n)
        printf("Radix sort completed\n");

        //long long temp2[outsize];
        //long long j = 0;
        long long j = 1;
        for (long long i = 0; i < outsize - 1; i++)
        {
            if (out[i] != out[i + 1])
            {
                //temp2[j++] = out[i];
                j++;
            }
        }
        //temp2[j++] = out[outsize - 1];
        printf("\n");
        printf("Final output: %lld\n", j);
        free(in);
    }
    else
    {
        MPI_Send(&outsize, 1, MPI_LONG_LONG, 0, 1, MPI_COMM_WORLD);
        MPI_Send(out, outsize, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD);
    }
    free(out);
    free(calc_rows);
    MPI_Finalize();
    return 0;
}
