BIXIE
=====

BIXIE is a optimized binary protocol for compressing XML files designed for the HomePort system. BIXIE can both encode and decode valid XML files with a schema. BIXIE has an almost flat compression rate of about 80%, and its performance is just as good as EXI with small data files and is twice as fast as the implementation of EXI called OpenEXI. However, as input files become larger, EXI achieves a greater compression rate than BIXIE. The implementation of BIXIE could be optimized, and the report describes multiple ways of doing so, one of them is to reduce the compiler phases to one instead of the current two.

In order to encode data you need:
  Expat (expat and libexpat1-dev)
  GCC
  make
  
In order to compile the test just do the following command:
  make
  
Or to enable verbose mode do the following command:
  make verbose
  
Thereafter run either ./bixie.out for the silent version or ./bixieVerbose.out for the verbose version.