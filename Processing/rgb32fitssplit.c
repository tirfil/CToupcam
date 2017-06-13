#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"

long width=0;
long height=0;

int write_fits(char* path,unsigned char* image)
{
	fitsfile *fptrout;
	unsigned int naxis = 2;
    long naxes[2];
    int status = 0;
    long nelements = width*height;
    naxes[0] = width;
	naxes[1] = height;
	fits_create_file(&fptrout,path, &status);
	fits_create_img(fptrout, BYTE_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TBYTE, 1, nelements, image, &status);
	fits_close_file(fptrout, &status);
}

int read_and_process(char* path)
{
	fitsfile *fptr;    
    int status = 0;
    unsigned int bitpix, naxis;
    long naxes[3];
    long nelements;
    int anynul;
    unsigned char* image;
    
	if (!fits_open_file(&fptr, path, READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
			if (naxis != 3){
				printf("Not a 3D fits\n");
				return -1;
			}
			
			if (bitpix != 8){
				printf("Process only 8 bit fits\n");
				return -1;
			}
			
			printf("bitpix=%d naxes=%d width=%ld height=%ld\n",bitpix,naxis,naxes[0],naxes[1]);
			nelements = naxes[0]*naxes[1];
			if (width==0 && height==0){
				width = naxes[0];
				height = naxes[1];
			} else {
				if (width != naxes[0] || height != naxes[1]){
					printf("naxes error: %ldx%ld instead of %ldx%ld\n",naxes[0],naxes[1],width,height);
					return -1;
				}
			}
			
			image = malloc(sizeof(unsigned char)*nelements);
			
			fits_read_img(fptr,TBYTE,1,nelements,NULL,image, &anynul, &status);
			write_fits("red.fits",image);
			fits_read_img(fptr,TBYTE,nelements +1,nelements,NULL,image, &anynul, &status);
			write_fits("green.fits",image);
			fits_read_img(fptr,TBYTE,2* nelements +1,nelements,NULL,image, &anynul, &status);
			write_fits("blue.fits",image);
			fits_close_file(fptr, &status);
			
			free(image);
						
			return status;
	
		  }
     }
     return -1;		
}

int main(int argc, char* argv[]) {
		if (argc != 2){
			printf("Usage: %s <rgb32 fits>\n",argv[0]);
			return -1;
		}
		return read_and_process(argv[1]);
}





