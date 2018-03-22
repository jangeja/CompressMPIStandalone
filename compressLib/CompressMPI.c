
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include "CompressMPI.h"
#include <miniz.h>
#include <snappy.h>
#include <lz4.h>


#define MAX_MESSAGE_BYTES 307200
#define MAX_CMP_BUF_BYTES LZ4_COMPRESSBOUND(MAX_MESSAGE_BYTES) + MAX_MESSAGE_BYTES

typedef struct {
  unsigned int inpBytes;
  unsigned int cmpBytes;
  unsigned char compAlg;
  char cmpBuf[MAX_CMP_BUF_BYTES];
} Packet;

#define RAW_PACKET_SIZE sizeof(unsigned int) * 2 + sizeof(char)
#define PACKET_SIZE sizeof(Packet)


int MPI_Csend(char *cBuf, long cBufLen, char *buf, long bufLen, int *dest, int numDest,  MPI_Comm comm,  const unsigned char compressAlg) {

  int returnStatus, i;
  Packet *packet = (Packet *)cBuf;
  double compStart, compEnd, sendStart, sendEnd;

  mz_ulong compressSize = (mz_ulong)cBufLen;

  size_t snappyCompSize;
  static int snappy_init = 0;
  static struct snappy_env env;
  if (!snappy_init) {
    snappy_init_env(&env);
    snappy_init = 1;
  }
  packet->inpBytes = bufLen;
  packet->compAlg = compressAlg;

  compStart = MPI_Wtime();
  if (compressAlg == LZ4) {  // lz4
    packet->cmpBytes = LZ4_compress_fast(buf, packet->cmpBuf, bufLen, cBufLen - RAW_PACKET_SIZE, 1);
  }
  else if (compressAlg == SNAPPY) {  // snappy
    snappyCompSize = (size_t)cBufLen - RAW_PACKET_SIZE;
    if (snappy_compress(&env, buf, bufLen, packet->cmpBuf, &snappyCompSize) < 0) {
      printf("Error in snappy compression\n");\
      exit(2);
    }
    packet->cmpBytes = (int)snappyCompSize;
  }
  else if (compressAlg == MINIZ) {  // miniz

    if (mz_compress((unsigned char *)packet->cmpBuf, &compressSize, (unsigned char *)buf, bufLen) != MZ_OK) {
      printf("Error in miniz compression\n");
      exit(3);
    }
    packet->cmpBytes = (int)compressSize;
  }
  else {
    printf("Error: unknown compress_alg flag\n");
    exit(packet->compAlg);
  }
  compEnd = MPI_Wtime();

  if (packet->cmpBytes < 1) {
    printf("Error compressing\n");
  }

  sendStart = MPI_Wtime();
  for (i = 0; i < numDest; i++) {
    returnStatus = MPI_Send(packet, RAW_PACKET_SIZE + packet->cmpBytes, MPI_CHAR, dest[i], 0, comm);
  }
  sendEnd = MPI_Wtime();
  printf("Compression Stats\n");
  printf("\tOriginal Data Size: %ld\n\tCompressed Data Size: %d\n\tCompression Ratio: %f\n", bufLen, packet->cmpBytes, ((float)bufLen) / packet->cmpBytes);
  printf("\tNumber of Sends: 2\n");
  printf("\tCompress Time: %f\n\tSend Time: %f\n\tSum of Time: %f\n", compEnd - compStart, sendEnd - sendStart, sendEnd - sendStart + compEnd - compStart);
  return returnStatus;
}


int MPI_Csend_Online(char *cBuf, long cBufLen, char *buf, long bufLen, int *dest, int numDest,  MPI_Comm comm,  const unsigned char compressAlg) {
  LZ4_stream_t* const lz4Stream = LZ4_createStream();
  Packet *packet = (Packet *)cBuf;
  int returnStatus = 0, i;
  long origSize = bufLen;
  long counter = 0, numSends = 1, compBytes = 0;
  double compStart, compEnd, compTotal = 0, sendStart, sendEnd, sendTotal = 0;
  static int snappy_init = 0;
  static struct snappy_env env;
  MPI_Request request;
  if (!snappy_init) {
    snappy_init_env(&env);
    snappy_init = 1;
  }

  mz_ulong compressSize = MAX_CMP_BUF_BYTES;
  size_t snappyCompSize;

  for (i = 0; i < numDest; i++) {
    MPI_Send(&bufLen, 1, MPI_LONG, dest[i], 0, comm);
  }

  while (bufLen > 0) {
    packet = (Packet *)cBuf;
    packet->compAlg = compressAlg;
    if (bufLen >= MAX_MESSAGE_BYTES) {
      packet->inpBytes = MAX_MESSAGE_BYTES;
    }
    else {
      packet->inpBytes = bufLen;
    }
    bufLen -= MAX_MESSAGE_BYTES;

    compStart = MPI_Wtime();
    if (compressAlg == LZ4) {  // lz4
      packet->cmpBytes = LZ4_compress_fast_continue(lz4Stream, buf, packet->cmpBuf, packet->inpBytes, MAX_CMP_BUF_BYTES, 1);
      if (packet->cmpBytes < 1) {
        printf("Error in lz4 compression\n");
        exit(1);
      }
    }
    else if (compressAlg == SNAPPY) {  // snappy
      snappyCompSize = MAX_CMP_BUF_BYTES;

      if (snappy_compress(&env, buf, packet->inpBytes, packet->cmpBuf, &snappyCompSize) < 0) {
        printf("Error in snappy compression\n");\
        exit(2);
      }
      packet->cmpBytes = (int)snappyCompSize;
    }
    else if (compressAlg == MINIZ) {  // miniz
      compressSize = MAX_CMP_BUF_BYTES;
      if (mz_compress((unsigned char *)packet->cmpBuf, &compressSize, (unsigned char *)buf, (mz_ulong)packet->inpBytes) != MZ_OK) {

        printf("Error in miniz compression\n");
        exit(3);
      }
      packet->cmpBytes = (int)compressSize;
    }
    else {
      printf("Error: unknown compress_alg flag\n");
      exit(compressAlg);
    }
    compEnd = MPI_Wtime();
    compBytes += packet->cmpBytes;

    compTotal += compEnd - compStart;
    if (packet->cmpBytes < 1) {
      printf("Error compressing, inpyBytes: %d, cmpBytes: %d\n", packet->inpBytes, packet->cmpBytes);
    }

    sendStart = MPI_Wtime();
    for (i = 0; i < numDest; i++) {
      returnStatus = returnStatus || MPI_Isend(packet, RAW_PACKET_SIZE + packet->cmpBytes, MPI_CHAR, dest[i], counter, comm, &request);
    }

    sendEnd = MPI_Wtime();
    sendTotal += sendEnd - sendStart;

    numSends++;

    counter ++;
    buf += packet->inpBytes;
    cBuf += RAW_PACKET_SIZE + packet->cmpBytes;
  }

  free(lz4Stream);
  printf("Compression Stats\n");
  printf("\tOriginal Data Size: %ld\n\tCompressed Data Size: %ld\n\tCompression Ratio: %f \n", origSize, compBytes, ((float)origSize) / compBytes);
  printf("\tNumber of Sends: %ld\n", numSends);
  printf("\tCompress Time: %f\n\tSend Time: %f\n\tSum of Time: %f\n", compTotal, sendTotal, compTotal + sendTotal);
  return returnStatus;
}

int MPI_Crecv(char *cBuf, long cBufLen, char *buf, long bufLen, int source, MPI_Comm comm) {
  int returnStatus = 0;
  double decompStart, decompEnd, recvStart, recvEnd;
  int packetSize;
  Packet *packet = (Packet *)cBuf;
  int lz4CmpBytes = 0;
  size_t snappyCmpBytes = 0;
  MPI_Status status;
  mz_ulong minizCmpBytes = 0;
  recvStart = MPI_Wtime();


  MPI_Probe(source, 0, MPI_COMM_WORLD, &status);
  MPI_Get_count(&status, MPI_CHAR, &packetSize);

  returnStatus = MPI_Recv(cBuf, packetSize, MPI_CHAR, source, 0, comm, &status);
  recvEnd = MPI_Wtime();

  decompStart = MPI_Wtime();

  if (packet->compAlg == LZ4) {  // lz4
    lz4CmpBytes = LZ4_decompress_safe(packet->cmpBuf, buf, packet->cmpBytes, packet->inpBytes);
  }
  else if (packet->compAlg == SNAPPY) {  // snappy
    if (!snappy_uncompressed_length (packet->cmpBuf, packet->cmpBytes, &snappyCmpBytes)) {
      printf("Error getting uncompressed length");
      exit(2);
    }
    if (snappy_uncompress(packet->cmpBuf, packet->cmpBytes, buf) < 0) {
      printf("Error in snappy compression\n");\
      exit(2);
    }
  }
  else if (packet->compAlg == MINIZ) {  // miniz
    minizCmpBytes = (mz_ulong)packet->inpBytes;
    if (mz_uncompress((unsigned char *)buf, &minizCmpBytes, (unsigned char *)packet->cmpBuf, packet->cmpBytes) != MZ_OK) {
      printf("Error in miniz compression\n");
      exit(3);
    }
  }
  else {
    printf("Error: unknown compress_alg flag\n");
    exit(packet->compAlg);
  }

  decompEnd = MPI_Wtime();

  if (lz4CmpBytes < 1 && snappyCmpBytes < 1 && minizCmpBytes < 1) {
    printf("Error decompressing\n");
    exit(-1);
  }
  printf("Decompression Stats\n");
  printf("\tNumber of receives: 2\n");
  printf("\tDecompress Time: %f\n\tRecv Time: %f\n\tSum of Time: %f\n", decompEnd - decompStart, recvEnd - recvStart, decompEnd - decompStart + recvEnd - recvStart);

  return returnStatus;

}

int MPI_Crecv_Online(char *buf, long bufLen, int source, MPI_Comm comm) {

  LZ4_streamDecode_t* const lz4Stream = LZ4_createStreamDecode();
  static Packet packet;
  size_t snappyCmpBytes;
  int returnStatus = 0, cmpBytes;
  long count, total = 0, numProbes = 0, numRecvs = 1;
  int packetSize;
  double probeStart, probeEnd, probeTotal = 0, countStart, countEnd, countTotal = 0;
  MPI_Status status;

  double recvStart, recvEnd, recvTotal = 0, decompStart, decompEnd, decompTotal = 0;
  mz_ulong uncompressSize;
  recvStart = MPI_Wtime();
  MPI_Recv(&count, 1, MPI_LONG, source, 0, comm, &status);
  recvEnd = MPI_Wtime();
  recvTotal += recvEnd - recvStart;

  while (total != count) {

    // Probeing and timing
    probeStart = MPI_Wtime();
    MPI_Probe(source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    probeEnd = MPI_Wtime();
    probeTotal += probeEnd - probeStart;

    // Get Count and Timing
    countStart = MPI_Wtime();
    MPI_Get_count(&status, MPI_CHAR, &packetSize);
    countEnd = MPI_Wtime();
    countTotal += countEnd - countStart;

    // Recv and timing
    recvStart = MPI_Wtime();
    returnStatus = returnStatus || MPI_Recv(&packet, packetSize, MPI_CHAR, source, status.MPI_TAG, comm, &status);
    recvEnd = MPI_Wtime();
    recvTotal += recvEnd - recvStart;

    decompStart = MPI_Wtime();

    if (packet.compAlg == LZ4) {  // lz4
      cmpBytes = LZ4_decompress_safe_continue(lz4Stream, packet.cmpBuf, buf + (status.MPI_TAG * MAX_MESSAGE_BYTES), packet.cmpBytes, packet.inpBytes);
    }
    else if (packet.compAlg == SNAPPY) {  // snappy
      if (!snappy_uncompressed_length (packet.cmpBuf, packet.cmpBytes, &snappyCmpBytes)) {
        printf("Error getting uncompressed length");
        exit(2);
      }
      cmpBytes = (int)snappyCmpBytes;
      if (snappy_uncompress(packet.cmpBuf, packet.cmpBytes, buf + (status.MPI_TAG * MAX_MESSAGE_BYTES)) < 0) {
        printf("Error in snappy compression\n");\
        exit(2);
      }
    }
    else if (packet.compAlg == MINIZ) {  // miniz
      uncompressSize = (mz_ulong)packet.inpBytes;
      if (mz_uncompress((unsigned char *)(buf + (status.MPI_TAG * MAX_MESSAGE_BYTES)), &uncompressSize,(unsigned char *)packet.cmpBuf, packet.cmpBytes) != MZ_OK) {
        printf("Error in miniz compression\n");
        exit(3);
      }
      cmpBytes = (int)uncompressSize;

    }
    else {
      printf("Error: unknown compress_alg flag\n");
      exit(packet.compAlg);
    }
    decompEnd = MPI_Wtime();
    decompTotal += decompEnd - decompStart;
    numProbes ++;
    numRecvs++;
    if (cmpBytes != packet.inpBytes) {
      // printf("Error!!!\n");
    }
    total += packet.inpBytes;
  }
  free(lz4Stream);
  printf("Decompression Stats\n");
  printf("\tNumber of receives: %ld\n\tNumber of Probes and getCounts: %ld\n", numRecvs, numProbes);
  printf("\tDecompress Time: %f\n\tRecv Time: %f\n\tProbe Time: %f\n\tGet Count Time: %f\n\tSum of Time: %f\n", decompTotal, recvTotal, probeTotal, countTotal, decompTotal + recvTotal + probeTotal + countTotal);
  return returnStatus;
}
