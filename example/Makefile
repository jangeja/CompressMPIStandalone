COMPRESS_DIR=$(PWD)/../compressLib

all: testSend

testSend: testSend.c
	mpicc -I $(COMPRESS_DIR) $(COMPRESS_DIR)/libcompressMPI.a testSend.c -o testSend

clean:
	rm testSend
