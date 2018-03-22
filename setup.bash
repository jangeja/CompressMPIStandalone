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

# Copy files to locations
cp compressLib/minizMakefile miniz/Makefile

echo "Building LZ4"
cd lz4 && make && cd ..

echo "Building snappy"
cd snappy && make && cd ..

echo "Building miniz"
cd miniz && make && cd ..
cd mpi && make && make install

echo "Building lib"
cd compressLib && make && cd ..

echo "ALL DONE"
echo ""
echo "CompressMPI library is in $PWD/compressLib"
