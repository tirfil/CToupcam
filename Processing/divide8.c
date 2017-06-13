#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <limits.h>
#include "fitsio.h"

long width=0;
long height=0;

int read_fits(char* path, unsigned char** image)
{
	fitsfile *fptr;    
    int status = 0;
    unsigned int bitpix, naxis;
    long naxes[2];
    long nelements;
    int anynul;
	if (!fits_open_file(&fptr, path, READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
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
			
			if (naxis != 2){
				printf("Process only 2D fits\n");
				fits_close_file(fptr, &status); 
				return -1;
			}

			if (bitpix != 8){
				printf("Process only 8 bit fits\n");
				fits_close_file(fptr, &status); 
				return -1;
			}
			
			*image = malloc(sizeof(unsigned char)*nelements);
			
			if (bitpix == 16){
				fits_read_img(fptr,TBYTE,1,nelements,NULL,*image, &anynul, &status);
				fits_close_file(fptr, &status);
				return status;
			} else {
				fits_close_file(fptr, &status);
				return -1;
			}
		  }
     }
     return -1;		
}


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

int main(int argc, char* argv[]) {
	unsigned char* a;
	unsigned char* b;
	unsigned char* c;
	long nelements;
	int i;
	int errors = 0;
	unsigned long lint_flat=0L;
	unsigned long lint_light=0L;
	unsigned char mini_flat=UCHAR_MAX;
	unsigned char maxi_flat=0;
	unsigned char mini_light=UCHAR_MAX;
	unsigned char maxi_light=0;
	unsigned char average_flat;
	unsigned char average_light;
	unsigned char delta_flat;
	unsigned char delta_light;
	unsigned char average;
	unsigned long factor;
	
	if (argc != 4){
		printf("Usage: %s <raw> <flat> <result>\n",argv[0]);
		return -1;
	}
	
	if (read_fits(argv[1],&a) == 0){
		if (read_fits(argv[2],&b) == 0){	
			nelements = width*height;
			c = malloc(sizeof(unsigned char)*nelements);
			// compute average value
			for(i=0;i<nelements;i++){
				lint_light += a[i];
				if (a[i] > maxi_light) maxi_light=a[i];
				if (a[i] < mini_light) mini_light=a[i];
			}
			for(i=0;i<nelements;i++){
				lint_flat += b[i];
				if (b[i] > maxi_flat) maxi_flat=b[i];
				if (b[i] < mini_flat) mini_flat=b[i];
			}
			average_flat = (unsigned char)(lint_flat/(unsigned long)nelements);
			average_light = (unsigned char)(lint_light/(unsigned long)nelements);
			printf("flat : average= %d min=%d max=%d\n",average_flat,mini_flat,maxi_flat);
			printf("light: average= %d min=%d max=%d\n",average_light,mini_light,maxi_light);
			delta_flat = maxi_flat - mini_flat;
			delta_light = maxi_light - mini_light;
			// algo 4
			factor = 65535L*(unsigned long)mini_flat/(unsigned long)maxi_light;
			lint_flat = (unsigned long)mini_light*factor/(unsigned long)maxi_flat;
			printf("factor=%ld min=%ld\n",factor,lint_flat);
			for(i=0;i<nelements;i++){
				lint_flat = (unsigned long)a[i]*factor/(unsigned long)b[i];
				c[i] = (unsigned char) lint_flat;
			}

			remove(argv[3]);
			if (write_fits(argv[3],c) == 0){
				free(a); free(b); free(c);
				return 0;
			}
		}
	}
	free(a); free(b); free(c);
	return -1;
	
}
