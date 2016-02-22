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
    int count = 1; // used for MPI_RECV; only one process id
    int N = 0; // first command line argument; range of primes
    int potentialPrime, numSieves;
    int end = - 1; // indicates end of program
    int manager = 0, vote = 0;
    int nextHolder;
    int state = true, len;

    /*Initialization phase*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // parse N
    if(sscanf(argv[1], "%i", &N)!=1){
        printf ("Error parsing argument");
    } 
    int maxLen = (int)((double)(N) / log(N)) * 2;
    int *sieves =  (int*)malloc(sizeof(int)*maxLen);
    int *allSieves =  (int*)malloc(sizeof(int)*maxLen);
    numSieves = 1;
    allSieves[0] = 2;
    int store[2] = {-1, -1}; 
    sieves[0] = 2;
    /* Main Body */
    if(pid == manager){
        // Manaager
        for(int num=3; num <= N; num+=2 ){
            vote = 0;
            //printf("process 0, broadcasted %d\n", num);
            nextHolder = ((numSieves - 1) % (numprocs - 1)) + 1;
            MPI_Bcast(&num, count, MPI_INT, manager, MPI_COMM_WORLD);
            //MPI_Bcast(&nextHolder, count, MPI_INT, manager, MPI_COMM_WORLD);
            MPI_Reduce(&state, &vote, count, MPI_INT, MPI_SUM, manager, MPI_COMM_WORLD);
            if(vote == numprocs){
                sieves[numSieves++] = num;
                store[0] = nextHolder;
                store[1] = num;
            }
            else{
                store[0] = -1;
                store[1] = -1;
            }
            MPI_Bcast(&store, 2, MPI_INT, manager, MPI_COMM_WORLD);
            //printf(">>> %d is %d\n", num, vote);
        }
        MPI_Bcast(&end, count, MPI_INT, pid, MPI_COMM_WORLD);
    }
    else{
        int i = 0;
        while(true){
            /* Receive number from the previous process*/
            MPI_Bcast(&potentialPrime, count, MPI_INT, manager, MPI_COMM_WORLD);
            //printf("Process %d broadcast received : %d\n", pid, potentialPrime);
            /* if end of process, break while*/
            if(potentialPrime == end){
                //printf("Process %d, received %d: Finishing...", pid, end);
                break;
            }
            //MPI_Bcast(&nextHolder, count, MPI_INT, manager, MPI_COMM_WORLD);
            for(i = 0; i < numSieves; i++){
                /* if it is a potential prime, pass to the next process */
                if(potentialPrime % sieves[i] == 0)
                    break;
            }
            state = false;
            /*If prime*/
            if( i == numSieves)
                state = true;
            MPI_Reduce(&state, &vote, count, MPI_INT, MPI_SUM, manager, MPI_COMM_WORLD);
            //printf("returning broadcast\n");
            MPI_Bcast(&store, 2, MPI_INT, manager, MPI_COMM_WORLD);
            if(store[0] > 0){
                if(store[0] == pid){
                    sieves[numSieves++] = store[1];
                    printf("Process %d: Received new sieve %d\n", pid, store[1]);
                } 
            }
        }
    }
    /*
    int *rcount;
    int counts[numprocs];
    MPI_Gather(&numSieves, 1, MPI_INT, counts, 1, MPI_INT, manager, MPI_COMM_WORLD);
    int disps[numprocs];
    disps[0] = 0;
    int s = counts[0];
    for (int i = 1; i < numprocs; i++){
       disps[i] = disps[i-1] + counts[i-1];
       s += counts[i];
    }
    int *all_paths = malloc((disps[numprocs-1] + counts[numprocs-1])*sizeof(int));
    MPI_Gatherv(sieves, numSieves, MPI_INT,
                 all_paths, counts, disps, MPI_INT, 0, MPI_COMM_WORLD);
    */
    int err = MPI_Barrier(MPI_COMM_WORLD);
    for(int i = 1; i < numprocs; i++)
        if(i == pid)
            printf(">>> Process %d holds %d primes\n", pid , numSieves - 1);

    err = MPI_Barrier(MPI_COMM_WORLD);
    if(pid == manager){
        printf("--- FOUND %d PRIMES ---\n", numSieves);
        for(int j = 0; j < numSieves; j++)
            printf("%d ", sieves[j]);
        printf("\n");
    }
    MPI_Finalize();
    return 0;
}
