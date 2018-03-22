# CompressMPIStandalone
Standalon library for Online and Offline sending in MPI

#Instructions for compiling

1. Clone this directory
```
git clone https://github.com/jangeja94/CompressMPIStandalone.git CompressMPIStandalone
```

2. Run setup script
```
./setup.bash
```

3. Everything is compiled!

4. This flags will need to be used when compiling your program
```
-I PATH_TO/CompressMPIStandalone/compressLib
```

5. You will also need to compile your program with the libcompressMPI.a file that is located in PATH_TO/CompressMPIStandalone/compressLib 

6. See the example folder for a sample program
