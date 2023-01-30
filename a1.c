#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gmp.h"
#include "mpi.h"

int main(int argc, char **argv)
{
    // unsigned long MAX_NUMBER = 10000000; // ten million
    unsigned long MAX_NUMBER = 1000000000; //one billion
    // unsigned long MAX_NUMBER = 1000000000000 //one trillion
    int NUM_REPS = 15; // increase to have fewer false positives. recommended range: 15-50
    int my_rank;
    int num_procs;
    int source;
    int dest = 0;
    int tag = 0;
    // message[0] = first prime in processor's split
    // message[1] = final prime in processor's split
    // message[2] = largest prime gap in processor's split
    // message[3] = beginning prime of largest prime gap in processor's split
    // message[4] = ending prime of largest prime gap in processor's split
    // example: a processor who receives the split 0-10 (2 3 5 7) should have a message {2, 7, 2, 3, 5}.
    unsigned long message[5];
    unsigned long my_start, n, my_prime_start, my_largest_prime_gap, my_prev_prime, largest_prime_gap, left_prime, right_prime; // to indicate the split per processor
    MPI_Status status;
    unsigned long *final_array;
    double start_time, end_time, time_elapsed;
    // branch start
    MPI_Init(&argc, &argv);
    // init mpz ints
    mpz_t my_cursor;
    mpz_init(my_cursor);
    // sync start times
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // find processor split
    n = (unsigned long)(MAX_NUMBER / num_procs);
    my_start = n * my_rank;
    // if max_num / num_procs is not perfectly divisible
    // example: num = 20, proc = 3, num%proc = 2
    // rank 0 and 1 (both are lower than num%proc) should get +1 to n, and +my_rank to my_start
    // rank 2 should get +(num%proc) to my_start
    //   post-process          |   pre-process
    //   0: 1 2 3 4 5 6 7      |   0: 1 2 3 4 5 6
    //   1: 8 9 10 11 12 13 14 |   1: 7 8 9 10 11 12
    //   2: 15 16 17 18 19 20  |   2: 13 14 15 16 17 18
    if (MAX_NUMBER % num_procs != 0)
    {
        if (my_rank < MAX_NUMBER % num_procs)
        {
            n += 1;
            my_start += my_rank;
        }
        else
        {
            my_start += MAX_NUMBER % num_procs;
        }
    }
    // find the first prime number, to calculate the gap between primes between splits
    mpz_set_ui(my_cursor, my_start); // this is equivalent to my_cursor = my_start
    while (mpz_probab_prime_p(my_cursor, NUM_REPS) == 0)
    {
        mpz_add_ui(my_cursor, my_cursor, 1); // this is equivalent to my_cursor++
    }
    my_prime_start = mpz_get_ui(my_cursor); // this is equivalent to my_prime_start = my_cursor

    // store my_prime_start in the first space in the array
    message[0] = my_prime_start;

    // get first prime gap
    // this assumes there are 2 primes in the processor split
    // why would there ever be only 1 prime in the processor split, so i will keep it this way
    my_prev_prime = my_prime_start;
    mpz_nextprime(my_cursor, my_cursor);
    my_largest_prime_gap = mpz_get_ui(my_cursor) - my_prev_prime;
    message[3] = my_prev_prime;
    message[4] = mpz_get_ui(my_cursor);

    // find largest prime gap and related information in split
    while (mpz_get_ui(my_cursor) < my_start + n)
    { // this is equivalent to my_cursor <= my_start +n
        my_prev_prime = mpz_get_ui(my_cursor);
        mpz_nextprime(my_cursor, my_cursor); // set my_cursor to the next prime
        if (mpz_get_ui(my_cursor) - my_prev_prime > my_largest_prime_gap)
        {
            my_largest_prime_gap = mpz_get_ui(my_cursor) - my_prev_prime;
            message[3] = my_prev_prime;
            message[4] = mpz_get_ui(my_cursor);
        }
    }
    // my_cursor would be the prime that is after my_start+n, so we use my_prev_prime;
    message[1] = my_prev_prime;
    message[2] = my_largest_prime_gap;
    printf("DEBUG: Process %d found largest prime gap %lu, between %lu and %lu, in split between %lu and %lu\n", my_rank, message[2], message[3], message[4], my_start, my_start + n);
    if (my_rank == 0)
    { // master proc
        int i;   
        // append all the messages to final array, starting with master proc's message
        final_array = malloc(5 * num_procs * sizeof(unsigned long));
        memcpy(final_array, message, 5 * sizeof(unsigned long));

        for (source = 1; source < num_procs; source++)
        {
            // mpi receive , then do same thing as above, but 1st argument of memcpy should be final array + (5*i)
            //                        this could be wrong, its either 5 or 40. 5 unsigned longs, or 40 bytes ^
            MPI_Recv(message, sizeof(5 * sizeof(unsigned long)), MPI_UNSIGNED_LONG, source, tag, MPI_COMM_WORLD, &status);
            printf("\nDEBUG: Message from processor %d: {%lu, %lu, %lu, %lu, %lu}", source, message[0], message[1], message[2], message[3], message[4]);
            memcpy(final_array + (5*source), message, 5 * sizeof(unsigned long));
        }

        //calculate all the prime gaps between splits, while simultaneously finding the largest prime gap
        largest_prime_gap = message[2]; //init largest_prime_gap to compare
        unsigned long a, b; //temp vars for processing
        for(int i = 0; i < 5*num_procs;i++) {
            if (i % 5 == 0 && i != 0) {
                //this is all the first primes in a processor's split, except the first one, which has no previous prime
                a = final_array[i];
                if (a - b > largest_prime_gap) {
                    largest_prime_gap = a - b;
                    left_prime = b;
                    right_prime = a;
                }
            } if (i % 5 == 1 && i != 5*num_procs-4) {
                //this is all the final primes in a processor's split, except the last one, which has no next prime
                b = final_array[i];
            } if (i % 5 == 2 && largest_prime_gap < final_array[i]) {
                //make 2 unsigned longs in vars at the top for left prime and right prime and save to it
                largest_prime_gap = final_array[i];
                left_prime = final_array[i+1];
                right_prime = final_array[i+2];
            } 
        }
        end_time = MPI_Wtime();
        time_elapsed = end_time - start_time;
        printf("\nThe largest gap between consecutive prime numbers under %lu is %lu, between %lu and %lu.\nTime elapsed:%f\n", MAX_NUMBER, largest_prime_gap, left_prime, right_prime, time_elapsed);
    }
    else
    { // slave proc
        MPI_Send(message, sizeof(5 * sizeof(unsigned long)), MPI_UNSIGNED_LONG, dest, tag, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}