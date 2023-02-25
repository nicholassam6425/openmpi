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
#include "mpi.h"

void printArray(int *arr, int len) {
    printf("{%d", arr[0]);
    for (int i = 1; i < len; i++) {
        printf(", %d", arr[i]);
    }
    printf("}\n");
}

int binarySearch(int arr[], int l, int r, int x)
{
    if (r >= l)
    {
        int mid = l + (r - l) / 2;

        if (arr[mid] == x)
        {
            return mid;
        }

        if (arr[mid] > x)
        {
            return binarySearch(arr, l, mid - 1, x);
        }

        return binarySearch(arr, mid + 1, r, x);
    }

    // if x is not found, return the next smallest number
    else
    {
        return r;
    }
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

    // parse array size input
    int ARRAY_SIZE = atoi(argv[1]);

    // mpi time yippee
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    int A[ARRAY_SIZE], B[ARRAY_SIZE];

    if (my_rank == 0)
    {
        // randomly generate arrays then sort them
        // root processor has to do this or else all the A's and B's are different
        srand(time(NULL));
        for (int i = 0; i < ARRAY_SIZE; i++)
        {
            A[i] = rand() % 1000;
            B[i] = rand() % 1000;
        }
        qsort(A, ARRAY_SIZE, sizeof(int), sortfunc);
        qsort(B, ARRAY_SIZE, sizeof(int), sortfunc);
        printf("A: ");
        printArray(A, ARRAY_SIZE);
        printf("B: ");
        printArray(B, ARRAY_SIZE);
    }
    // root proc sends A and B to all other procs
    MPI_Bcast(A, ARRAY_SIZE, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(B, ARRAY_SIZE, MPI_INT, 0, MPI_COMM_WORLD);

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
            my_rank += my_rank;
        }
        else
        {
            my_start += ARRAY_SIZE % num_procs;
        }
    }
    int highest_num_index = n + my_start;
    int my_B_end = binarySearch(B, 0, ARRAY_SIZE - 1, A[highest_num_index]);
    int my_B_start;
    // proc 0 tells proc 1 what it's B end is, and that will become proc 1's B start. repeat for all procs
    if (my_rank == 0) //root proc (proc 0)
    {
        my_B_start = 0;
        MPI_Send(&my_B_end, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }
    else if (my_rank == num_procs - 1) //last proc (proc p-1)
    {
        MPI_Recv(&my_B_start, 1, MPI_INT, my_rank-1, 0, MPI_COMM_WORLD, NULL);
    }
    else //every other proc
    {
        MPI_Recv(&my_B_start, 1, MPI_INT, my_rank-1, 0, MPI_COMM_WORLD, NULL);
        MPI_Send(&my_B_end, 1, MPI_INT, my_rank+1, 0, MPI_COMM_WORLD);
    }
    //merge A and B 
    printf("%d: {A: %d - %d, B: %d - %d}\n", my_rank, my_start, highest_num_index, my_B_start, my_B_end);

    MPI_Finalize();
    return 0;
}
