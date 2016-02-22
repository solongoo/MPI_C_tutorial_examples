/*
 * Author: Solongo Munkhjargal
 * CS580
 *
 * Sieve of Eratosthenes 
 * _____________________
 * https://en.wikipedia.org/wiki/Prime_number_theorem
 * Prime number theorem states that there are roughly x / log(x) below x; 
 * */
#include "math.h"
#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

typedef enum { false, true } bool;

int main(int argc, char* argv[]){
    int numprocs; // the number of processes
    int pid; // current process id
    MPI_Status status; // status object
    int tag = 1; // random tag, not used
    int count = 1; // used for MPI_RECV; only one process id
    int N = 0; // first command line argument; range of primes
    int potentialPrime, sieve;
    int end = - 1; // indicates end of program

    /*Initialization phase*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // parse N
    if(sscanf(argv[1], "%i", &N)!=1){
        printf ("Error parsing argument");
    } 
    /* Main Body */
    if(pid == 0){
        /* Distributor process: Send odd numbers to process 1 */
        for(int num=3; num <= N; num+=2){
            MPI_Send(&num, count, MPI_INT, pid + 1, tag, MPI_COMM_WORLD); 
            printf("process 0, sending %d\n", num);
        }
        // signal that all numbers have been passed 
        MPI_Send(&end, count, MPI_INT, pid + 1, tag, MPI_COMM_WORLD);
        int maxPrimeCount = 2 * (int)(ceil((double)(N) / log((double)(N))));
        int *all = (int*)malloc(sizeof(int)*maxPrimeCount);
        int i = 1;
        all[0] = 2;
        while(true){
            MPI_Recv(&sieve, count, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
            if(sieve != -1)
                all[i++] = sieve;
            else
                break;
        }
        printf("--- FOUND %d PRIMES ---\n", i);
        for(int j = 0; j < i; j++)
            printf("%d ", all[j]);
        printf("\n");
    }
    else if(pid < numprocs - 1){
        /* if intermediate process, store only one sieve*/
        MPI_Recv(&sieve, count, MPI_INT, pid - 1, tag, MPI_COMM_WORLD, &status);  
        while(true){
            /* Receive number from the previous process*/
            MPI_Recv(&potentialPrime, count, MPI_INT, pid - 1, tag, MPI_COMM_WORLD, &status);
            /* if end of process, break while*/
            if(potentialPrime == end){
                MPI_Send(&end, count, MPI_INT, pid + 1, tag, MPI_COMM_WORLD);
                printf("Process %d, received %d: Finishing...", pid, end);
                break;
            }
            /* if it is a potential prime, pass to the next process */
            if(potentialPrime % sieve != 0){
                MPI_Send(&potentialPrime, count, MPI_INT, pid + 1, tag, MPI_COMM_WORLD);
                printf("Process %d: Sending %d to process %d\n", pid, potentialPrime, pid + 1);
            }
            /* Not a prime*/
            else
                printf("Process %d: %d is not a prime\n", pid, potentialPrime);
        }
        /*Finishing; send the sieve to process 0*/
        printf("Finishing \n>>>I am process  %d. My prime is: %d\n", pid, sieve); 
        MPI_Send(&sieve, count, MPI_INT, 0, tag, MPI_COMM_WORLD);
    }
    else{
        /* Sieve process */
        int maxPrimeCount = 2 * (int)(ceil((double)(N) / log((double)(N))));

        /* List of primes for this process */
        int *sieves = (int*)malloc(sizeof(int)*maxPrimeCount);
        int mySievesCount = 0;
        int i;

        while(true){
            /*Receive potential prime number*/
            MPI_Recv(&potentialPrime, count, MPI_INT, pid - 1, tag, MPI_COMM_WORLD, &status);
            printf("process %d, received %d\n", pid, potentialPrime);
            if(potentialPrime == end)
                break;
            /*If first prime, then append to list of primes*/
            for(i = 0; i < mySievesCount; i++){
                // if not prime
                if(potentialPrime % sieves[i] == 0)
                    break;
            }
            // if prime, pass to the next process
            if(i == mySievesCount)
                    sieves[mySievesCount++] = potentialPrime;
        }
        printf(">>>I am process  %d: My primes are : ", pid);
        for(int i = 0; i< mySievesCount; i++){
            printf("%d ", sieves[i]);   
            MPI_Send(sieves + i, count, MPI_INT, 0, tag, MPI_COMM_WORLD); 
        }
        printf("\n");
        MPI_Send(&end, count, MPI_INT, 0, tag, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}
