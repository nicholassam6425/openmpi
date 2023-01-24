#include <stdio.h>
#include <string.h>
#include "gmp.h"
#include "mpi.h"
/*
Notes: gmp uses unsigned longs for their number calculations so we need to do casts between ints and unsigned longs
operations between ui and int result in a ui


*/
int main(int argc, char *argv[]) {
    int MAX_NUMBER = 1000000000; //one billion
    // int MAX_NUMBER = 1 000 000 000 000 //one trillion
    int NUM_REPS = 15 //increase to have fewer false positives. recommended range: 15-50
    int my_rank;
    int num_procs;
    int source;
    int dest;
    //message[0] = first prime in processor's split
    //message[1] = final prime in processor's split
    //message[2] = largest prime gap in processor's split
    //message[3] = beginning prime of largest prime gap in processor's split
    //message[4] = ending prime of largest prime gap in processor's split
    //example: a processor who receives the split 0-10 (2 3 5 7) should have a message {2, 7, 2, 3, 5}. 
    unsigned long message[5];
    unsigned long my_start, n, my_prime_start, my_largest_prime_gap, my_prev_prime, largest_prime_gap; //to indicate the split per processor
    MPI_Status status;
    unsigned long *final_array, *real_final_array;
    double start_time, end_time, time_elapsed;

    //branch start
    MPI_Init(&argc, &argv);

    //init mpz ints
    mpz_t my_cursor;
    mpz_init(my_cursor);

    //sync start times
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    //find processor split
    n = (unsigned long)(MAX_NUMBER / num_procs);
    my_start = n * my_rank;
    //if max_num / num_procs is not perfectly divisible
    //example: num = 20, proc = 3, num%proc = 2
    //rank 0 and 1 (both are lower than num%proc) should get +1 to n, and +my_rank to my_start
    //rank 2 should get +(num%proc) to my_start
    //  post-process          |   pre-process
    //  0: 1 2 3 4 5 6 7      |   0: 1 2 3 4 5 6
    //  1: 8 9 10 11 12 13 14 |   1: 7 8 9 10 11 12
    //  2: 15 16 17 18 19 20  |   2: 13 14 15 16 17 18
    if (MAX_NUMBER % num_procs != 0) {
        if (my_rank < MAX_NUMBER % num_procs) {
            n += 1;
            my_rank += my_rank;
        } else {
            my_start += MAX_NUMBER % num_procs;
        }
    } 
    
    //find the first prime number, to calculate the gap between primes between splits
    mpz_set_ui(my_cursor, my_start) //this is equivalent to my_cursor = my_start
    while (mpz_probab_prime_p(my_cursor, NUM_REPS) == 0) {
        mpz_add(my_cursor, (unsigned long)(1)); //this is equivalent to my_cursor++
    }
    my_prime_start = mpz_get_ui(my_cursor); //this is equivalent to my_prime_start = my_cursor

    //store my_prime_start in the first space in the array
    message[0] = my_prime_start;

    //get first prime gap
    //this assumes there are 2 primes in the processor split
    //why would there ever be only 1 prime in the processor split, so i will keep it this way
    my_prev_prime = my_prime_start;
    mpz_nextprime(my_cursor, my_cursor);
    my_largest_prime_gap = mpz_get_ui(my_cursor) - my_prev_prime;
    message[4] = my_prev_prime;
    message[5] = mpz_get_ui(my_cursor);

    //find largest prime gap and related information in split
    while (mpz_get_ui(my_cursor) <= my_start + n) { //this is equivalent to my_cursor <= my_start +n
        my_prev_prime = mpz_get_ui(my_cursor);
        mpz_nextprime(my_cursor, my_cursor); //set my_cursor to the next prime
        if (mpz_get_ui(my_cursor) - my_prev_prime > my_largest_prime_gap) {
            my_largest_prime_gap = mpz_get_ui(my_cursor) - my_prev_prime;
            message[4] = my_prev_prime;
            message[5] = mpz_get_ui(my_cursor);
        }
    }
    //my_cursor would be the prime that is after my_start+n, so we use my_prev_prime;
    message[1] = my_prev_prime;
    message[2] = my_largest_prime_gap;

    if (my_rank == 0) { //master proc
        //so we append all the messages to final array, starting with master proc's message
        final_array = malloc(5 * num_procs * sizeof(unsigned long));
        memcpy(final_array, message, 5 * sizeof(unsigned long));
        for(int i = 1; i < num_proc; i++) {
            //mpi receive , then do same thing as above, but 1st argument of memcpy should be final array + (5*i)
            //                        this could be wrong, its either 5 or 40. 5 unsigned longs, or 40 bytes ^
        }
        
        
        //calculate all the prime gaps between splits, while simultaneously finding the largest prime gap
        largest_prime_gap = message[2] //init largest_prime_gap to compare
        unsigned long a, b; //temp vars for processing
        for(int i = 0; i < 5*num_procs;i++) {
            if (i % 5 == 0 && i != 0) {
                //this is all the first primes in a processor's split, except the first one, which has no previous prime

                a = message[i];
            } else if (i % 5 == 1 && i != 5*num_procs-1) {
                //this is all the final primes in a processor's split, except the last one, which has no next prime
                
                b = message[i]
            } else if (i % 5 == 2 && largest_prime_gap < message[i]) {
                //make 2 unsigned longs in vars at the top for left prime and right prime and save to it
                largest_prime_gap = message[i]
                //left = message[i+1]
                //right = message[i+2]
            } else if (a - b > largest_prime_gap) {
                largest_prime_gap = a - b;
                //left = b
                //right = a
            }
        }

    }
    else { //slave proc
        MPI_Send(message, sizeof(message), MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);
    }
    end_time = MPI_Wtime();
    time_elapsed = end_time - start_time;
    if (my_rank == 0) {
        printf("\n%d", time_elapsed);
    }
    MPI_Finalize();
}