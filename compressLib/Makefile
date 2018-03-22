LZ4_DIR=$(PWD)/../lz4/lib
MINIZ_DIR=$(PWD)/../miniz
SNAPPY_DIR=$(PWD)/../snappy

LZ4_LIB=$(LZ4_DIR)/liblz4.a
LIBSNAPPY_SO=$(SNAPPY_DIR)/libsnappyc.so.1

all: CompressMPI.o allObjs.o libcompressMPI.a

libcompressMPI.a: allObjs.o
	libtool -static -o libcompressMPI.a allObjs.o

allObjs.o: CompressMPI.o $(SNAPPY_DIR)/snappy.o $(MINIZ_DIR)/miniz.o $(MINIZ_DIR)/miniz_tdef.o $(MINIZ_DIR)/miniz_tinfl.o $(MINIZ_DIR)/miniz_zip.o $(LZ4_DIR)/lz4.o
	ld -r $^ -o $@

CompressMPI.o: CompressMPI.c
	mpicc -I $(LZ4_DIR)/lib -I $(MINIZ_DIR) -I $(SNAPPY_DIR) -c CompressMPI.c

clean:
	rm CompressMPI.o libcompressMPI.a allObjs.o
