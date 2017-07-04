#! /bin/bash -v
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
gcc track.c -L/usr/local/lib -ltoupcam -o track 
gcc test.c -lcfitsio -o test
