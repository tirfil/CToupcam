#! /bin/bash -v
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
gcc test.c -lcfitsio -o test
gcc track.c -L/usr/local/lib -ltoupcam -o track 
gcc track1.c -L/usr/local/lib -ltoupcam -o track1 

