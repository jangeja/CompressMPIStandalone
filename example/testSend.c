#include "mpi.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <CompressMPI.h>

#define MAX_BUFFER 1000000000

int main(int argc, char *argv[]) {
  MPI_Init(NULL, NULL);

  int rank, numProcs, rv, originalLen, compress_alg = 1;
  long fsize;
  FILE *f;
  char filename[15];
  double sendStart, sendEnd, recvStart, recvEnd;
  int dest[] = {1};

  if (argc >= 4) { // get compression alg, defaults to lz4
    if (!strcmp(argv[3], "lz4")) {
       compress_alg = LZ4;
    }
    else if (!strcmp(argv[3], "snappy")) {
       compress_alg = SNAPPY;
    }
    else if (!strcmp(argv[3], "miniz")) {
       compress_alg = MINIZ;
    }
    else {
       printf("Unknown compression alg. Please specify lz4, snappy, or miniz\n");
       exit(1);
    }
  }

  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
  MPI_Request request;
  MPI_Status status;

  // Get the rank of the process
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // char original[] = "LZ compression is based on finding repeated strings: Five, six, seven, eight, nine, fifteen, sixteen, seventeen, fifteen, sixteen, seventeen.";
  // originalLen = strlen(original) + 1;
  char *decompressed = malloc(MAX_BUFFER);
  char *compressed = malloc(MAX_BUFFER);

  printf("Rank: %d\n", rank);
  if (rank == 0) {
    f = fopen(argv[1], "r");
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    char *contents = malloc(fsize + 1);
    fread(contents, fsize, 1, f);
    fclose(f);
    printf("FSIZE: %lu\n", fsize);
   //printf("Arg1: %s, Arg2: %s, Arg3: %s\n", argv[1], argv[2], argv[3]);

    if (!strcmp(argv[2], "online")) {
      printf("Online Compression\n");
      //Online compression
      MPI_Csend_Online(compressed, MAX_BUFFER, contents, fsize, dest, 1,  MPI_COMM_WORLD, compress_alg);
    }
    else if (!strcmp(argv[2], "offline")) {
      // Offline compression
      printf("Offline Compression\n");
      MPI_Csend(compressed, MAX_BUFFER, contents, fsize, dest, 1, MPI_COMM_WORLD, compress_alg);
    }
    else {
      //No Compression
      printf("No Compression\n");
      sendStart = MPI_Wtime();
      MPI_Send(&fsize, 1, MPI_LONG, 1, 0, MPI_COMM_WORLD);
      MPI_Send(contents, fsize, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
      sendEnd = MPI_Wtime();
      printf("\tOriginal Data Size: %ld\n\tSend Time: %f\n\tNumber of Sends: 2\n", fsize, sendEnd - sendStart);
    }

  }
  else if (rank == 1) {
    if (!strcmp(argv[2], "online")) {
      //Online Compression
      MPI_Crecv_Online(decompressed, MAX_BUFFER, 0, MPI_COMM_WORLD);

    }
    else if (!strcmp(argv[2], "offline")) {
      //Offline Compression
      MPI_Crecv(compressed, MAX_BUFFER, decompressed, MAX_BUFFER, 0, MPI_COMM_WORLD);

    }
    else {
      //No Compression
      MPI_Recv(&fsize, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD, &status);
      MPI_Recv(decompressed, fsize, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
      printf("\tRecv Time: %f\n\tNumber of recvs: 2\n", sendEnd - sendStart);


    }
    printf("Received...\n");
  }




  MPI_Finalize();
  return 0;
}
