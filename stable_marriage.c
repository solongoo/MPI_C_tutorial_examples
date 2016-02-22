#include "time.h"
#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#define COORD 0
typedef enum { false, true } bool;

int main(int argc, char* argv[]){
    int numprocs; // the number of processes
    int pid; // current process id
    int N, count = 1, tag = 1, start = 1;
    MPI_Status status; // status object

    /*Initialization phase*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    N = numprocs / 2;  // integer division rounds down
    int menPartners[N];
    int run = 1;
    if(pid == COORD){
        // coordinator
        int menPartnerValues[N], bestPartnerVal, bestPartner;
        int counter[N], stable = false;
        int woman, womanID, manID;
        int packet[2];
        int start = 1;
        /* Artificial Barrier: Used for printing ranks before matching */ 
        for(int i = 1; i <= 2*N; i++){
            MPI_Send(&start, count, MPI_INT, i, tag, MPI_COMM_WORLD);
            MPI_Recv(&start, count, MPI_INT, i, tag, MPI_COMM_WORLD, &status);
        }
        printf("\n\nCOORDINATOR: MATCHING STARTING\n\n");
        for(int i = 1; i <= 2*N; i++)
            MPI_Send(&start, count, MPI_INT, i, tag, MPI_COMM_WORLD);
        while(true){
            // receive reports from men }
            for(int i = 1; i <= N; i++){
                MPI_Recv(packet, 2, MPI_INT, i, tag, MPI_COMM_WORLD, &status);
                menPartners[i - 1] = packet[0];
                menPartnerValues[i - 1] = packet[1];
            }
            printf("COORDINATOR: RUN %d complete - received reports from all men\n", run++);
            //count men who thinks he is engaged to woman }
            memset(counter, 0, N*sizeof(int));
            for(int woman = 0; woman < N; woman++){
                if(menPartners[woman] > -1){
                    counter[menPartners[woman]]++;
                }
            }
            //Determine stability and find out if any men was dumped
            stable = true;
            for(int woman = 0; woman < N; woman ++){
                if(counter[woman] != 1){
                    stable = false;
                    // find the best partner
                    bestPartnerVal = -1;
                    for(int manID = 0; manID < N; manID++){
                        // man thinks he is engaged to woman
                        if(menPartners[manID] == woman){
                            // woman thinks he is the best partner for her
                            if(menPartnerValues[manID] > bestPartnerVal){
                                bestPartner = manID;
                                bestPartnerVal = menPartnerValues[manID]; 
                            }
                        }
                    }
                    // Dump others
                     for(int manID = 0; manID < N; manID++){
                        if(menPartners[manID] == woman){
                            if( manID != bestPartner)
                                menPartners[manID] = -1;
                        }
                    }
                }
            }
            //update men
            for(int manID = 0; manID < N; manID++){
                packet[0] = menPartners[manID];
                packet[1] = stable;
                //printf("COORDINATOR: Updating MAN %d with partner id : %d and stability : %d\n", manID + 1, menPartners[manID], stable);
                MPI_Send(packet, 2, MPI_INT, manID + 1, tag, MPI_COMM_WORLD);
            }
            //if stable shut down women
            if(stable == true){
                /*Make sure all men are shut down*/
                for(int manID = 0; manID < N; manID++){
                    MPI_Recv(&start, count, MPI_INT, manID + 1, tag, MPI_COMM_WORLD, &status);
                }
                /*
                printf("\n\nCOORDINATOR: SYSTEM STABILIZED AFTER %d RUNS\n", run - 1);
                for(int i = 0; i < N; i++)
                    printf("======= MAN %d is paired with WOMAN %d ========\n", i, menPartners[i]); */
                printf("COORDINATOR: Men are shut down\n");
                for(int woman = 1; woman <= N; woman++)
                    MPI_Send(&stable, count, MPI_INT, N + woman, tag, MPI_COMM_WORLD);
                printf("COORDINATOR: Women are shut down\n");
                break;
            }
        }
    }
    else if(pid >= 1 && pid <= N){
        // Man
        int womensRank[N], answer, stability, proposalCount = 0;
        int partnerValue = -1; // woman's rank for the man
        int partnerID = -1,  engaged = false;
        int packet[2];
        srand(pid);
        // rank women
        for(int i = 0; i < N; i++)
            womensRank[i] =  i + 1;
        //Shuffle
        for(int i = 0 ; i < N ; i ++){
            int temp = womensRank[i];
            int ind = rand() % N;
            womensRank[i] = womensRank[ind];
            womensRank[ind] = temp;
        }
        MPI_Recv(&start, count, MPI_INT, COORD, tag, MPI_COMM_WORLD, &status);
        printf("MAN %d: RANKING ", pid);
        for(int i = 0; i < N; i++)
            printf("%d ", womensRank[i]);
        printf("\n");
        MPI_Send(&start, count, MPI_INT, COORD, tag, MPI_COMM_WORLD);
        MPI_Recv(&start, count, MPI_INT, COORD, tag, MPI_COMM_WORLD, &status);
        while(true){
            if(engaged == false && proposalCount < N){
                // get the id of the highest ranked woman
                int w, rank = -1;
                for(int i = 0 ; i < N; i++){
                    if(womensRank[i] > rank){
                        w = i;
                        rank = womensRank[i];
                    }
                }
                // propose
                printf("MAN %d: proposing WOMAN %d\n", pid, w + 1);
                MPI_Send(&womensRank[w], count, MPI_INT, N + w + 1, tag, MPI_COMM_WORLD);
                // woman responds with her rank of the man
                MPI_Recv(&partnerValue, count, MPI_INT, N + w + 1, tag, MPI_COMM_WORLD, &status);
                if(partnerValue != false){
                    engaged = true;
                    partnerID = w;
                    printf(">>> MAN %d: engaged to WOMAN %d\n", pid, partnerID + 1);
                }
                else{
                    // cannot propose this woman again
                    womensRank[w] = -1;
                    printf("___ MAN %d: rejected by WOMAN %d\n", pid, partnerID + 1);
                }
                proposalCount++;
            }
            //Report current match to the coordinator
            packet[0] = partnerID;
            packet[1] = partnerValue;
            //printf("MAN %d: reporting to COORDINATOR my partner id : %d and partner Value : %d\n", pid, partnerID + 1, partnerValue);
            MPI_Send(packet, 2, MPI_INT, COORD, tag, MPI_COMM_WORLD);
            // update from coordinater
            MPI_Recv(packet, 2, MPI_INT, COORD, tag, MPI_COMM_WORLD, &status);
            int partnerIDstatus = packet[0];
            stability = packet[1];
            if(stability == true)
                break;
            if(partnerIDstatus == -1){
                engaged = false;
                womensRank[partnerID] = -1;
                partnerID = -1;
                partnerValue = -1;
            }
            else
                partnerID = partnerIDstatus;
            //printf("MAN %d: received from COORDINATOR partnerID : %d\n", pid, partnerID + 1);
        }
        //printf("---- MAN %d: DONE\n", pid);
        MPI_Send(&start, count, MPI_INT, COORD, tag, MPI_COMM_WORLD);
    }
    else{
        // woman
        int mensRank[N],coordinatorID = 0,engaged = false;
        int partnerID = -1, currentRank = -1, currentPartnerID = -1;
        int newMenID, newRank, answer;
        srand(pid);
        // rank women
        for(int i = 0; i < N; i++)
            mensRank[i] =  i + 1;
        //Shuffle
        for(int i = 0 ; i < N ; i ++){
            int temp = mensRank[i];
            int ind = rand() % N;
            mensRank[i] = mensRank[ind];
            mensRank[ind] = temp;
        }
        MPI_Recv(&start, count, MPI_INT, COORD, tag, MPI_COMM_WORLD, &status);
        printf("WOMAN %d: RANKING ", pid - N);
        for(int i = 0; i < N; i++)
            printf("%d ", mensRank[i]);
        printf("\n"); 
        MPI_Send(&start, count, MPI_INT, COORD, tag, MPI_COMM_WORLD);
        MPI_Recv(&start, count, MPI_INT, COORD, tag, MPI_COMM_WORLD, &status);
        while(true){
            // receive proposal
            MPI_Recv(&newRank, count, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
            // coordinator signals end of program
            newMenID = status.MPI_SOURCE;
            if(newMenID == COORD)
                break;
            printf("??? WOMAN %d: received proposal from %d\n", pid - N , newMenID);
            // if not engaged or finds better man, say yes to new man
            if(engaged == false || (mensRank[newMenID - 1] > mensRank[partnerID])){
                engaged = true;
                partnerID = newMenID;
                answer = mensRank[newMenID - 1];
                printf(">>> WOMAN %d: accepting %d\n", pid - N, newMenID);
            }
            else{
                answer = false;
                printf("!!! WOMAN %d: rejecting %d\n", pid - N, newMenID);
            }
            MPI_Send(&answer, count, MPI_INT, newMenID, tag, MPI_COMM_WORLD);
        }
        //printf("---- WOMAN %d: DONE\n", pid);
    }
    if(pid == COORD){
        printf("\n\nCOORDINATOR: SYSTEM STABILIZED AFTER %d RUNS\n", run - 1);
        for(int i = 0; i < N; i++)
            printf("======= MAN %d is paired with WOMAN %d ========\n", i, menPartners[i]);
    } 
    MPI_Finalize();
    return 0;
}
