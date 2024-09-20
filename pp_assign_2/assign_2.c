#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define n 5  // Matrix Size(n*n)

// Function to perform Dijkstra's algorithm in parallel
void HW2(int matrix[n][n], int output[n], int process_id, int total_processes) {
    int i, count, leastVal, leastPos;
    int done[n];// to keep tracking the finalized distance 
    MPI_Status status;

    
    for (i = 0; i < n; i++) {
        done[i] = 0;
        output[i] = matrix[0][i];  // Initialize output with direct distances from node 0
    }
    // Initializing done and distances array
    done[0] = 1;  // Starting at node 0.
    count = 1;  // Count the number of nodes processed 

    while (count < n) {
        //1.Find the node with the smallest distance (min distance).
        int localLeastVal = 987654321;
        int localLeastPos = -1;
        //Each of the process will calculate the local minimum distance to a node that is not processed yet.
        //The process will do this by iterating nodes and find's the minimum distance which has not been processed 
        for (i = 0; i < n; i++) {
            if (!done[i] && output[i] < localLeastVal) {
                localLeastVal = output[i];
                localLeastPos = i;
            }
        }

        // 2. after each processor finds the local minimum distance,Using MPI_Allreduce we will find the overall minimum distance among all the processes
        //that global minimum is stored in  leastval
        MPI_Allreduce(&localLeastVal, &leastVal, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        

        int localLeastPosArray[total_processes];
        localLeastPosArray[process_id] = localLeastPos;
        //MPI_Allgather is used to collect localleastpositions and storing them in an array which determines which node is corresponding to the global minimum distance.
        MPI_Allgather(&localLeastPosArray[process_id], 1, MPI_INT, localLeastPosArray, 1, MPI_INT, MPI_COMM_WORLD);

        // to find the global least position
        leastPos = -1;
        for (i = 0; i < total_processes; i++) {
            if (localLeastPosArray[i] != -1 && output[localLeastPosArray[i]] == leastVal) {
                leastPos = localLeastPosArray[i];
                break; //
            }
        }

        
        done[leastPos] = 1;  // Flagging the node (that it is visited)
        count++;

        // 3. Parallel update of distances.
        for (i = process_id; i < n; i += total_processes) {
            if (!done[i]) {
                int newDist = leastVal + matrix[leastPos][i];
                if (newDist < output[i]) {
                    output[i] = newDist;
                }
            }
        }

        //Now MPI_Allreduce is used to make sure each process has the most recent values of the shortest path distances after each updation of the cycle.
        MPI_Allreduce(MPI_IN_PLACE, output, n, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    }
}

// Main function
int main(int argc, char **argv) {
    int process_id, total_processes;
    int matrix[n][n];
    int output[n];
    double starting_time,ending_time;
    
    MPI_Init(&argc, &argv);//Intializes the mpi environment
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);//retrived the processor_id of the current process s
    MPI_Comm_size(MPI_COMM_WORLD, &total_processes);//Number of processes.

    // User Input adjacency matrix.
    if (process_id == 0) {
        printf("Enter the 5x5 matrix (0 is assigned to diagonal elements):\n");  // code is written in such a way that diagonal elements are default 0
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if(i==j){
                    matrix[i][j]=0;
                }
                else{
                    scanf("%d", &matrix[i][j]);  //User Input only the non-fiagonal elements
                }
                
            }
        }
    }

    //broadcasts a message from the process with rank root to all processes of the group, itself include
    int i=0;
    while(i<n) {
        MPI_Bcast(matrix[i], n, MPI_INT, 0, MPI_COMM_WORLD);
        i++;
    }

    
    starting_time = MPI_Wtime();//start time

    //HW2-djkitra's algorithm
    HW2(matrix, output, process_id, total_processes);


    ending_time = MPI_Wtime();//stop time
    i=0;

    //print the outcome of the shortest path from the root process.
    if (process_id == 0) {
        printf("\nShortest distances from node 0:\n");
        while(i<n){
            printf("Node 0 to Node %d: %d\n", i, output[i]);
            i++;
        }

        printf("Computed shortest path time: %f seconds\n", ending_time - starting_time);
    }

    MPI_Finalize();
    return 0;
}
