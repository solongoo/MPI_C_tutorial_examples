/*
 * Author: Solongo Munkhjargal
 * CS580
 *
 * Roller Coaster
 * command line arguments: C, R
 * C - number of passengers a car can hold at a time
 * R - the number of runs each passengers take
 * ______________
 * >>> Default setting
 * A car holds C = 2 passengers
 * There are four passenger processes
 * Each passenger takes R = 2 runs
 * Car takes one second to go around the track 
 * Passengers wait for random milliseconds before making next board request;
 * */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

typedef enum { false, true } bool;

double waitTime(){
    return (double)rand() / (double)RAND_MAX ;
}

int main(int argc, char* argv[]){
    int numprocs; // the number of processes
    int pid; // current process id
    int C = 2;   // At a time car can hold two passengers 
    int R = 2;  // Each passanger takes 10 rides
    int numPassengers = 0; // the number of passengers currently in the car
    int car = 0;  // car process id
    MPI_Status status; // status object
    int tag = 0; // random tag, not used
    int count = 1; // used for MPI_RECV; only one process id
    int unboard = 1; // signals passengers to unboard
    int passengers[C]; // passengers in car
    int unboardSignal, donePassengers, breakWhile, runNum = 0; 
    int carTime = 1; // time to go around track once

    /*Initialization phase*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    
    if(sscanf(argv[1], "%i", &C)!=1){
        printf ("Error parsing C");
    }
    if(sscanf(argv[2], "%i", &R)!=1){
        printf ("Error parsing R");
    }
    /* Main Body */
    if(pid == 0){
        // Car process
        donePassengers = 0;
        memset(passengers, 0, C*sizeof(int));
        while(true){
            numPassengers = 0; // new ride; empty car
            breakWhile = false;
            runNum++;
            printf("\n <<< RUN : %d >>> \n", runNum);
            printf("--- Boarding passengers\n");
            /*Passengers are boarding*/
            for(int index=0; index < C; index++){
                MPI_Recv(&passengers[index], count, MPI_INT,
                            MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                /*If tag indicates the passenger has completed R runs*/
                if(status.MPI_TAG == true){
                    donePassengers += 1;
                    if(donePassengers == numprocs - 1){
                        breakWhile = true;
                        break;
                    }
                    index--;
                }
            }
            /*If all passengers are done*/
            if(breakWhile==true){
                printf("NO passenger found!\n");
                printf("<<< DONE >>>\n");
                break;      
            }
            printf("--- Car is full\n");
            printf("----- Current passengers: ");
            for(int i = 0; i < C; i++){
                printf("%d ", passengers[i]);
            }
            printf("\n");
            /* go around the track*/
            printf("----- Car is going around...\n");
            sleep(carTime);
            /* unboard passengers */
            printf("--- Done...\n");
            printf("--- Unboarding Passengers...\n");
            for(int index=0; index < C; index++){
                MPI_Send(&unboard, count, MPI_INT, 
                        passengers[index], tag, MPI_COMM_WORLD);
            }
            printf("--- Run Complete...\n");
        }
    }        
    else{
        // Passenger process
        int runs;
        for(runs = 0; runs < R; runs++){
            // board 
            MPI_Send(&pid, count, MPI_INT, car, 0, MPI_COMM_WORLD);
            // unboard 
            MPI_Recv(&unboardSignal, count, MPI_INT, car, tag, MPI_COMM_WORLD, &status);
            //wait
            sleep(waitTime());
        }
        /* I am done */
        MPI_Send(&pid, count, MPI_INT, car, 1, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}
