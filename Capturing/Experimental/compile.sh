#! /bin/bash -v
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
gcc MonoCapture12.c -lcfitsio -L/usr/local/lib -ltoupcam -o MonoCapture12 
gcc RgbCapture12.c -lcfitsio -L/usr/local/lib -ltoupcam -o RgbCapture12 


