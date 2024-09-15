#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#define generate_data(i,j) (i)+(j)*(j)

int main(int argc, char **argv){
    int i,j,process_id,np,mtag; //process_id=Processor_id, np=No_of_processor, mtag = to distinguish between different messages being sent or receieved
    double t0, t1 ;
    int data[100][100], row_sum[100] ;
    int size_of_chunk=5; // Define chunk size as a variable , chunk is nothing but how many rows are processed or sent at once.
    double starting_time, ending_time;
    double communication_time = 0.0, computation_time = 0.0;
    MPI_Status status;//Source, tag or error code of the message.
    MPI_Request req_s, req_r;

    MPI_Init(&argc, &argv);//Intializes the mpi environment 
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);//retrived the processor_id of the current process 
    MPI_Comm_size(MPI_COMM_WORLD, &np);//Number of processes.

    if(process_id==0){
        /*
        generating data for rows 0-49 and sending to the processor_id=1 in chunks of size 5.
        generates data and computes the row summation for rows 50-99, while receiving computed row sums of rows 0-49 from process 1.
        */
        for (i = 0; i < 50; i++)
            for (j = 0; j < 100; j++)
                data[i][j] = generate_data(i, j);
        /*generate_data(i,j) is calculated using the formula (i+j^2)*/
        
        mtag = 1;
        /*Send data in chunks of size "5"  rows to process_id 1*/
        int i=0;
        while(i<50){
            starting_time = MPI_Wtime();
            MPI_Send(&data[i][0], size_of_chunk * 100, MPI_INT, 1, mtag, MPI_COMM_WORLD);
            ending_time = MPI_Wtime();
            communication_time += (ending_time - starting_time);
            i+=size_of_chunk;
        }

        //Returns an elapsed time on the calling processor
        /*calculating the time taken to send the data for every chunk and adding it to obtain the value of the communication time*/

        
        /* row_sums computation for 50–99, where process 0 does the row sum computation directly*/
        for (i = 50; i < 100; i++) {
            for (j = 0; j < 100; j++) {
                data[i][j] = generate_data(i, j);
            }
            row_sum[i] = 0;
            for (j = 0; j < 100; j++) {
                row_sum[i] += data[i][j];
            }
        }

        mtag = 2;
        MPI_Recv(row_sum, 50, MPI_INT, 1, mtag, MPI_COMM_WORLD, &status);
        /*Process 0 uses MPI_Recv with mtag = 2 to obtain the computed row sums for rows 0-49 from Process 1.*/
        for(i=0; i<100; i++) {
            printf(" %d ", row_sum[i]) ;
            if(i%10 == 9) printf("\n");
        }

        
        /* printing row sums*/

        printf("Process 0 Communication time: %f seconds\n", communication_time);



    }
    else { 
        /*
        This process is in responsible of: 
        Receiving data in chunks from Process 0.Row sums for rows 0-49 are calculated while communication and computation are overlapped.Sending to Process 0 with the computed row sums.
        
        */
        
        mtag = 1;

        // Start to receive the very first chunk. 
        MPI_Irecv(&data[0][0], size_of_chunk * 100, MPI_INT, 0, mtag, MPI_COMM_WORLD, &req_r);
        /*
            In order to enable Process 1 to begin receiving the initial chunk of data while it is still performing calculations, MPI_Irecv is utilized for data non-blocking.
        */

        for (i = 0; i < 50; i += size_of_chunk) {
            // While the current chunk is being received, calculate the row sums of the preceding chunk if this is not the first iteration.

            /*
            As the new chunk is being received in the background, the procedure computes the row sums for the previous chunk.
            Before moving on to the following computation, MPI_Wait makes sure that the data has been received in whole.
            */
            if (i > 0) {
                starting_time = MPI_Wtime();
                for (int k = i - size_of_chunk; k < i; k++) {
                    row_sum[k] = 0;
                    for (j = 0; j < 100; j++) {
                        row_sum[k] += data[k][j];
                    }
                }
                ending_time = MPI_Wtime();
                computation_time += (ending_time - starting_time);
            }

            // Wait for the current chunk to be fully received before proceeding
            MPI_Wait(&req_r, &status);


            // Start receiving the next chunk (non-blocking)
            if (i + size_of_chunk < 50) {
                MPI_Irecv(&data[i + size_of_chunk][0], size_of_chunk * 100, MPI_INT, 0, mtag, MPI_COMM_WORLD, &req_r);
            }
        }

        /* Compute the row sums for the last chunk (rows 45-49).
        Row sums for the final chunk (rows 45–49) are calculated by the procedure after all chunks have been received.
        */
        starting_time = MPI_Wtime();
        for (int k = 50 - size_of_chunk; k < 50; k++) {
            row_sum[k] = 0;
            for (j = 0; j < 100; j++) {
                row_sum[k] += data[k][j];
            }
        }
        ending_time = MPI_Wtime();
        computation_time += (ending_time - starting_time);

        // returns the calculated row sums with tag mtag = 2 to Process 0.
        mtag = 2;
        starting_time = MPI_Wtime();
        MPI_Send(row_sum, 50, MPI_INT, 0, mtag, MPI_COMM_WORLD);
        ending_time = MPI_Wtime();
        communication_time += (ending_time - starting_time);

        printf("process 1 communication time: %f seconds\n", communication_time);
        printf("process 1 Computation time: %f seconds\n", computation_time);
    }


    /* 
    process 0: prints their communication time along with computed row sums for all rows
    process 1: prints only the communication time

    
    */
    MPI_Finalize();
    return 0;


    


}