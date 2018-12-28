#! /bin/bash -v
gcc rgb32fitssplit.c -lcfitsio -o rgb32fitssplit 
gcc median8.c -lcfitsio -o median8
gcc substract8.c -lcfitsio -o substract8
gcc divide8.c -lcfitsio -o divide8
gcc center2.c -lm -lcfitsio -o center2 
gcc center3.c -lm -lcfitsio -o center3 
gcc bayer2rgb.c -lcfitsio -o bayer2rgb
gcc bayer2rgbbin2x2.c -lcfitsio -o bayer2rgbbin2x2
gcc bayer2awb.c -lcfitsio -o bayer2awb
gcc bayer2rgbc.c -lcfitsio -lm -o bayer2rgbc
