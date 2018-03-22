enum {
   LZ4,
   SNAPPY,
   MINIZ
};

int MPI_Csend(char *cBuf, long cBufLen, char *buf, long bufLen, int *dest, int numDest,  MPI_Comm comm,  const unsigned char compressAlg);

int MPI_Crecv(char *cBuf, long cBufLen, char *buf, long bufLen, int source, MPI_Comm comm);

int MPI_Csend_Online(char *cBuf, long cBufLen, char *buf, long bufLen, int *dest, int numDest,  MPI_Comm comm,  const unsigned char compressAlg);

int MPI_Crecv_Online(char *buf, long bufLen, int source, MPI_Comm comm);
