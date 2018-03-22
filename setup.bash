# Cloning all repos
if [ ! -d lz4 ]; then
  git clone https://github.com/lz4/lz4.git lz4
fi

if [ ! -d snappy ]; then
  git clone https://github.com/andikleen/snappy-c.git snappy
fi

if [ ! -d miniz ]; then
  git clone https://github.com/richgel999/miniz.git miniz
fi

# Copying all files to relevant dirs
CUR_DIR=$PWD

LIBS_TO_ADD="-Wno-error=unused-command-line-argument $CUR_DIR/miniz/miniz.o $CUR_DIR/miniz/miniz_zip.o $CUR_DIR/miniz/miniz_tinfl.o $CUR_DIR/miniz/miniz_tdef.o $CUR_DIR/snappy/libsnappyc.so.1 $CUR_DIR/lz4/lib/liblz4.a "

FLAGS_TO_ADD="-Wno-error=unused-command-line-argument -I $CUR_DIR/miniz -I $CUR_DIR/snappy -I $CUR_DIR/lz4/lib "

# Copy files to locations
cp compressLib/minizMakefile miniz/Makefile

echo "Building LZ4"
cd lz4 && make && cd ..

echo "Building snappy"
cd snappy && make && cd ..

echo "Building miniz"
cd miniz && make && cd ..
cd mpi && make && make install

echo "ALL DONE"
echo "run source ~/.bashrc or source ~/.bash_profile"
