#! /bin/bash -v
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
gcc MonoCapture8.c -lcfitsio -L/usr/local/lib -ltoupcam -o MonoCapture8 
gcc RgbCapture8.c -lcfitsio -L/usr/local/lib -ltoupcam -o RgbCapture8 
gcc RawCapture12.c -lcfitsio -L/usr/local/lib -ltoupcam -o RawCapture12 
gcc RawPushCapture12.c -lcfitsio -L/usr/local/lib -ltoupcam -o RawPushCapture12 

