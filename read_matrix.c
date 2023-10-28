#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define dim_cart_comm 2
#define tag 0
#define VERBOSE 0
#define INF 99999

int matrixDim, **Matrix;

int fox_alg(int, int, int, int**, int, int**, int**, int*);

int min(int v1, int v2){
    return v1<v2 ? v1 : v2;
}

void min_plus_matrix(int* c, int* a, int* b, int blk){
  int i,j,k;
  
  for (i = 0; i < blk; i++)
    for (j = 0; j < blk; j ++)
      for (k = 0; k < blk; k++)
        c[i*blk+j] = min(c[i*blk+j], (a[i*blk+k] + b[k*blk+j]));
}

void print_matrix(int* Matrix[], int dim) {
    for (int i=0; i<dim; i++) {
        for (int j=0; j<dim; j++) {
            fprintf(stderr, "%d ", Matrix[i][j]);
        }
        fprintf(stderr, "\n");
    }
}


// Given a dimention this function reads a matrix from that dim*dim inputs
void read_matrix(int dim, int* Matrix[]) {
    if (VERBOSE) {
        fprintf(stderr, "Reading matrix of size (%d)\n", matrixDim);
    }
    for (int i=0; i<dim; ++i) {
        for (int j=0; j<dim; ++j) {
            scanf("%d", &Matrix[i][j]);
        }
    }
    if (VERBOSE) {
        fprintf(stderr, "Finished reading input\n");
    }
}


int main(int argc, char **argv) {
    // Create an int and a char variable
    int numprocs, rank, numblocks, blocksize;
    int *M[4];  // A_ij = M[0]  B_hk = M[1] C = M[2]  TMP  = M[3]     (These are all blocks [sub matrixes]) 
    long startTime = time(NULL); 

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Reading matrix from INPUT
    if (rank == 0) {
        scanf("%d", &matrixDim);
        if (VERBOSE) {
            fprintf(stderr, "Matrix of size (%d, %d)\n", matrixDim, matrixDim);
        }
        Matrix = (int **)malloc(matrixDim * sizeof(int *));
        for (int i=0; i<matrixDim; i++) 
            Matrix[i] = (int *)malloc(matrixDim * sizeof(int));
        read_matrix(matrixDim, Matrix);
    }

    // Sharing initial matrix size to all processes to initialize 
    MPI_Bcast(&matrixDim, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Validating number of processes for matrix size
    if (numprocs % (int) sqrt(matrixDim) != 0) {
        if (rank==0){    
            fprintf(stderr, "nproc = %d | MatrixSize = %d. We must have p=m*m and matrix_size %% m = 0.\n",  numprocs, matrixDim);
            for (int i = 0; i < matrixDim; ++i) {
                free(Matrix[i]);
            }
            free(Matrix);
            fprintf(stderr, "Exiting with error after %ld seconds\n", time(NULL) - startTime);
        }
        MPI_Finalize();
        return 0;
    }

    // Initialize "Matrix" in processes which are not the "master" process
    if (rank != 0) {
        Matrix = (int **)malloc(matrixDim * sizeof(int *));
        for (int i=0; i<matrixDim; i++) 
            Matrix[i] = (int *)malloc(matrixDim * sizeof(int));
    }

    // Sharing initial matrix size to all processes to initialize 
    for (int i=0; i<matrixDim; i++) {
        MPI_Bcast(Matrix[i], matrixDim, MPI_INT, 0, MPI_COMM_WORLD);
    }


    // Allocating memory for final result matrix
    // int *MatrixResult[matrixDim];  // A_ij = M[0]  B_hk = M[1] Cxy = M[2]  TMP  = M[3]     (These are all blocks [sub matrixes]) 
    // for (int i=0; i<matrixDim; i++) {
    //     MatrixResult[i] = (int *)malloc(matrixDim * sizeof(int));
    // }
    int* MatrixResult = (int *)malloc(matrixDim*matrixDim * sizeof(int));

    // Calculations for array sizes
    numblocks = sqrt(numprocs);         // Find how many blocks we'll have (num_process = numblocks*numblocks)
    blocksize = matrixDim / numblocks;  // Finding the size of each block

    
    // Initializing each blocks sub matrixes (A, B, Result and a temporary one)
    for (int i=0; i<4; i++) {
        if (i==3) {
            M[i] = (int *)malloc(sizeof(int) * blocksize * blocksize);
            for (int j=0; j<blocksize*blocksize; j++) { M[i][j] = INF;}
        }
        else {
            M[i] = (int *)calloc(sizeof(int), blocksize * blocksize);
        }
    }
    if (!(M[0] && M[1] && M[2] && M[3])) {
        fprintf(stderr,  "Out of memory!\n");
        for(int i = 0; i < 4; i++) 
            free(M[i]);
        goto exit;
    }

    for (int step=0; step<matrixDim-2; step++) {
        if (VERBOSE) {fprintf(stderr, "Process %d: Starting Fox_alg\n", rank);}
        fox_alg(rank, numblocks, blocksize, M, matrixDim, Matrix, Matrix, MatrixResult); 
        if (VERBOSE) {fprintf(stderr, "Process %d: Finished Fox_alg\n", rank);}
        MPI_Bcast(MatrixResult, matrixDim*matrixDim, MPI_INT, 0, MPI_COMM_WORLD);
        
        
        for (int i=0; i<matrixDim; i++) {
            for (int j=0; j<matrixDim; j++) {
                Matrix[i][j] = MatrixResult[i*matrixDim + j];
            }
        }
        for (int i=0; i<4; i++) {
            for (int j=0; j<blocksize*blocksize; j++) {
                if (i==3) { M[i][j] = INF;}
                else {M[i][j] = 0;}
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (VERBOSE || rank==0) {
        fprintf(stderr, "Process %d - Final Matrix\n", rank);

        // print_matrix(MatrixResult, matrixDim);
        for (int i=0; i<matrixDim; i++) {
            for (int j=0; j<matrixDim; j++) {
                fprintf(stderr, "%d ", MatrixResult[i*matrixDim + j]);
            }
            fprintf(stderr, "\n");
        }
    }

    for (int i = 0; i < matrixDim; ++i) {free(Matrix[i]);}
    free(Matrix);
    MPI_Finalize();
    return 0;

exit:
    fprintf(stderr, "Exiting with error after %ld seconds\n", time(NULL) - startTime);
    MPI_Finalize();
    return 0;
}

int fox_alg(int rank, int numblocks, int blocksize, int* M[], int matrixDim, int** M0, int** M1, int* MR) {
    if (VERBOSE) {
        fprintf(stderr, "   <fox_alg> Process %d: Inside fox_alg. Vars: \n       NumBlocks=%d | BlockSize=%d | M[0][0]=%d | MatrixDim=%d | M0[0][0]=%d | M1[0][0]=%d | MR[0][0]=\n", rank, numblocks, blocksize, M[0][0], matrixDim, M0[0][0], M1[0][0]);
    }
    
    int i, j, myRow, myCol;
    MPI_Comm gridComm, rowComm, colComm;

    int  sizes[dim_cart_comm] = { numblocks, numblocks };
    int   wrap[dim_cart_comm] = { 1, 1 };
    int colDir[dim_cart_comm] = { 1, 0 };
    int rowDir[dim_cart_comm] = { 0, 1 };
    int optm = 1, gridRank, gridCoords[dim_cart_comm], row, col;
    int dst, src, blocksquare = blocksize*blocksize;
    MPI_Status status;

    // Communicator creation based on a topology, we are using the comm_world and dividing it. basicamente é uma grid
    // Podemos ter que criar um novo comunicador caso não queiramos usar o comum, will see
    MPI_Cart_create(MPI_COMM_WORLD, dim_cart_comm, sizes, wrap, optm, &gridComm);

    // Attributing a gridRank for process inside the gridComm
    MPI_Comm_rank(gridComm, &gridRank);
    
    // "Coordenadas" dos processos given o rank
    MPI_Cart_coords(gridComm, gridRank, dim_cart_comm, gridCoords);

    // Dividing comunicator into rows and columns
    MPI_Cart_sub(gridComm, colDir, &colComm);   // Esta a partir o comuncador em colunas
    MPI_Cart_sub(gridComm, rowDir, &rowComm);   // Esta a partir o comuncador em linhas

    // My coordinates in the Cart Grid
    myRow = gridCoords[0];
    myCol = gridCoords[1];

    if (VERBOSE) {fprintf(stderr, "   <fox_alg> Process %d - Initial grid coords (%d,%d) | Rank %d\n", rank, myRow, myCol, gridRank);}
    
    for(i = 0; i < blocksize; i++) {
        // get_block_row(**M0, matrixDim, myRow, myCol, i, blocksize, &(M[0][i*blocksize]));
        for (j=0; j<blocksize; j++) {
            M[0][i*blocksize + j] = M0[myRow*blocksize + i][myCol*blocksize + j];
        }
        // get_block_row(**M0, matrixDim, myRow, myCol, i, blocksize, &(M[0][i*blocksize]));
        for (j=0; j<blocksize; j++) {
            M[1][i*blocksize + j] = M1[myRow*blocksize + i][myCol*blocksize + j];
        }
    }
    if (VERBOSE) {
        fprintf(stderr, "   <fox_alg> Process %d - Matrix[0] first step:\n", rank);
        for (int i=0; i<blocksize; i++) {
            for (int j=0; j<blocksize; j++) {
                fprintf(stderr, "       <fox_alg> Process %d - M[0][%d, %d] = %d ", rank, i, j, M[0][i*blocksize + j]);
            }
            fprintf(stderr, "\n");
        }
    }

    
    src = ( myRow + 1 ) % numblocks;            // Process Source in our cart grid (neighbour)
    dst = ( myRow + numblocks - 1) % numblocks; // Process Destination in our cart grid (neighbour)

    if(VERBOSE)
      fprintf(stderr, "Clone %d has src = %d and dst = %d\n", gridRank, src, dst);

    for (i = 0; i < numblocks; i++) {
        int root = ( myRow  + i ) % numblocks;
        if(VERBOSE) 
            fprintf(stderr, "    <fox_alg> Process %d This is my root %d\n", rank, root);
        if(root == myCol){
            //snd my matrix A to neighbour
            if (VERBOSE) {fprintf(stderr, "    <fox_alg> Process %d sending matrix A to neighbour\n", root);}
            MPI_Bcast(M[0], blocksquare, MPI_INT, root, rowComm);
            min_plus_matrix(M[2], M[0], M[1], blocksize); 
        } else {
            //rcv an A from neighbour
            if (VERBOSE) {fprintf(stderr, "    <fox_alg> Process %d receiving matrix A from neighbour\n", root);}
            MPI_Bcast(M[3], blocksquare, MPI_INT, root, rowComm);
            min_plus_matrix(M[2], M[3], M[1], blocksize); 
        }
 
        //   rotate B's 
        MPI_Sendrecv_replace(M[1], blocksquare, MPI_INT, dst, tag, src, tag, colComm, &status);

    }

    if (VERBOSE) {fprintf(stderr, "Clone %d computed:\n\n", rank);}

    // Writing C
    MPI_Gather(M[2], blocksquare, MPI_INT, MR, blocksquare, MPI_INT, 0, MPI_COMM_WORLD);

    // Switch order of matrix result
    if (rank==0) {
        int *tmp = (int *)malloc(matrixDim*matrixDim * sizeof(int));
        int next_elem = 0;
        for (int n=0; n<numblocks*numblocks; n++) {
            for (int i=0; i<blocksize; i++) {
                for (int j=0; j<blocksize; j++) {
                    int row = (int) n / numblocks;
                    int col = n % numblocks;
                    tmp[i*matrixDim + j + col*blocksize + row*blocksize*matrixDim] = MR[next_elem];
                    next_elem++;
                }
            }
        }
        for (int i=0; i<matrixDim*matrixDim; i++) {
            MR[i] = tmp[i];
        }
    }
    return 0;
}