/*
1. split array A into [logn] sized arrays
maybe split into n/p instead

likely parallel can start here

2. for each array you just split A into, find the largest index in array B where it is smaller or equal to the largest number in the split A arrays.
example:
split A: [[1,2,3],[4,5,6]]
B: 1,2,5,6,7,8
j (output): [1,3]
B[j[0]] = 2 (highest index of B where it is smaller than the largest number in A[0])
B[j[1]] = 6 (same as above but in A[1])
this creates partitions where A_i ... A_ilogn and B_i ... B_j[i] cover the same range of numbers, with no overlap, and where the range for i is always less than i+1

3. merge within each partition, then merge all partitions
*/
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "mpi.h"

void printArray(int *arr, int len)
{
    printf("{%d", arr[0]);
    for (int i = 1; i < len; i++)
    {
        printf(", %d", arr[i]);
    }
    printf("}\n");
}

int binarySearch(int arr[], int l, int r, int x)
{
    // if (r >= l)
    // {
    //     int mid = l + (r - l) / 2;

    //     if (arr[mid] == x)
    //     {
    //         return mid;
    //     }

    //     if (arr[mid] > x)
    //     {
    //         return binarySearch(arr, l, mid - 1, x);
    //     }

    //     return binarySearch(arr, mid + 1, r, x);
    // }

    // // if x is not found, return the next smallest number
    // else
    // {
    //     return r;
    // }

    while (l <= r)
    {
        int m = l + (r - l) / 2;
        // Check if x is present at mid
        if (arr[m] == x)
            return m;
        // If x greater, ignore left half
        if (arr[m] < x)
            l = m + 1;
        // If x is smaller, ignore right half
        else
            r = m - 1;
    }
    //return the number to the left if not found
    return r;
}
// returns true if number is a positive number
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

// sort function for qsort. sorts in ascending order
int sortfunc(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

int main(int argc, char *argv[])
{
    if (argc != 2 && isNumber(argv[1]))
    {
        printf("Usage: %s array_size\narray_size must be a positive integer", argv[0]);
    }

    int my_rank, num_procs;
    double start_time, start_time2, end_time, time_elapsed;
    // parse array size input
    int ARRAY_SIZE = atoi(argv[1]);

    // mpi time yippee
    MPI_Init(&argc, &argv);

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    int A[ARRAY_SIZE], B[ARRAY_SIZE];

    if (my_rank == 0)
    {
        printf("Executing parallel merge on %d processors\n", num_procs);
    }
    printf("Processor %d check\n", my_rank);
    if (my_rank == 0)
    {
        // randomly generate arrays then sort them
        // root processor has to do this or else all the A's and B's are different
        srand(time(NULL));
        for (int i = 0; i < ARRAY_SIZE; i++)
        {
            A[i] = rand() % __INT_MAX__;
            B[i] = rand() % __INT_MAX__;
        }
        printf("Arrays generated\n");
        qsort(A, ARRAY_SIZE, sizeof(int), sortfunc);
        qsort(B, ARRAY_SIZE, sizeof(int), sortfunc);
        printf("Arrays sorted\n");
    }
    start_time2 = MPI_Wtime();
    // root proc sends A and B to all other procs
    if (my_rank == 0)
    {
        for (int i = 1; i < num_procs; i++) {
            printf("Proc 0 sending to %d", i);
            MPI_Send(A, ARRAY_SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(B, ARRAY_SIZE, MPI_INT, i, 1, MPI_COMM_WORLD);
        }
    }
    else
    {
        MPI_Recv(A, ARRAY_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
        MPI_Recv(B, ARRAY_SIZE, MPI_INT, 0, 1, MPI_COMM_WORLD, NULL);
        printf("Proc %d received A & B\n", my_rank);
    }
    if (my_rank == 0)
    {
        printf("Successfully broadcast A & B to all processors");
    }
    // for the time being im doing n/p so that it's an even split between procs.
    // i think if you just change n to logn or something it should work?
    int n = ARRAY_SIZE / num_procs;
    int my_start = n * my_rank;

    // copypasted this from a1, it should work.
    if (ARRAY_SIZE % num_procs != 0)
    {
        if (my_rank < ARRAY_SIZE % num_procs)
        {
            
            n += 1;
            my_start += my_rank;
        }
        else
        {
            my_start += ARRAY_SIZE % num_procs;
        }
    }
    int highest_num_index = n + my_start - 1;
    int my_B_end = binarySearch(B, 0, ARRAY_SIZE - 1, A[highest_num_index]);
    my_B_end += 1;
    int my_B_start;
    printf("Proc %d binary search successful", my_rank);

    // proc 0 tells proc 1 what it's B end is, and that will become proc 1's B start. repeat for all procs
    if (my_rank == 0) // root proc (proc 0)
    {
        printf("%d sending info\n", my_rank);
        my_B_start = 0;
        MPI_Send(&my_B_end, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }
    else if (my_rank == num_procs - 1) // last proc (proc p-1)
    {
        MPI_Recv(&my_B_start, 1, MPI_INT, my_rank - 1, 0, MPI_COMM_WORLD, NULL);
        printf("%d received info\n", my_rank);
        my_B_start += 1;
        my_B_end = ARRAY_SIZE;
    }
    else // every other proc
    {
        MPI_Recv(&my_B_start, 1, MPI_INT, my_rank - 1, 0, MPI_COMM_WORLD, NULL);
        printf("%d received info\n", my_rank);
        my_B_start += 1;
        printf("%d sending info\n", my_rank);
        MPI_Send(&my_B_end, 1, MPI_INT, my_rank + 1, 0, MPI_COMM_WORLD);
    }
    // merge A and B
    int *sub_C;
    int size = (my_B_end - my_B_start) + (highest_num_index - my_start) + 2;

    printf("%d: {A: %d - %d, B: %d - %d, size: %d}\n", my_rank, my_start, highest_num_index, my_B_start, my_B_end, size);
    if (my_B_end != -1)
    {
        sub_C = malloc(size * sizeof(int));
        int i = my_start, j = my_B_start, k = 0;
        while (i <= n + my_start && j <= my_B_end)
        {
            if (A[i] < B[j])
            {
                sub_C[k++] = A[i++];
            }
            else
            {
                sub_C[k++] = B[j++];
            }
        }
        while (i <= n + my_start)
        {
            sub_C[k++] = A[i++];
        }
        while (j <= my_B_end)
        {
            sub_C[k++] = B[j++];
        }
    }
    else if (my_B_end == -1)
    {
        sub_C = malloc((n + 1) * sizeof(int));
        int j = 0;
        for (int i = my_start; i <= n + my_start; i++)
        {
            sub_C[j++] = A[i];
        }
        size = n + 1;
    }
    printf("Proc %d merged successfully.\n", my_rank);
    if (my_rank == 0)
    {
        MPI_Status status;
        int *C;
        C = malloc(2 * ARRAY_SIZE * sizeof(int));
        memcpy(C, sub_C, size * sizeof(int));
        int C_index = size;
        printf("Proc %d received Proc %d successfully\n", my_rank, my_rank);
        for (int i = 1; i < num_procs; i++)
        {
            MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_INT, &size);
            sub_C = realloc(sub_C, size * sizeof(int));
            MPI_Recv(sub_C, size, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
            memcpy(C + C_index, sub_C, size * sizeof(int));
            C_index += size;
            printf("Proc %d received Proc %d successfully\n", my_rank, i);
        }

        end_time = MPI_Wtime();
        time_elapsed = end_time - start_time;
        printf("Time elapsed (including sorting initial array): %f\n", time_elapsed);
        time_elapsed = end_time - start_time2;
        printf("Time elapsed (excluding sorting initial array): %f\n", time_elapsed);
    }
    else
    {
        MPI_Send(sub_C, size, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}