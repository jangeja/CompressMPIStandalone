# Test Send program

## Instructions for running

1. You must have mpi installed
2. Compile the program
```
make
```
3. Execute the program
```
mpiexec -np 2 testSend FILE TRANSMISSION COMPRESSION_ALG
```
* FILE - the file to be sent
* TRANSMISSION - the type of compressions to do (none, online, offline)
* COMPRESSION_ALG - the compression algorithm to use (lz4, snappy, miniz)


## Running on 127 lab machines
1. This can be executed on the lab machines with the provided LABMNGR.py file

2. Execute the LABMNGR.py script that is in this example folder
```
./LABMNGR.py testSend FILE TRANSMISSION COMPRESSION_ALG
```
