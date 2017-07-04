#include <string.h>
#include <stdio.h>
#include "fitsio.h"


int star16(unsigned short *image, unsigned int width, unsigned int height, int* X, int* Y)
{
    long nelements;	
	int x,y;
    int i;
    unsigned short mini, maxi;
    unsigned short threshold;
    int count, xmaxcount,ymaxcount;
    int xtmp,ytmp,xxmax,xymax,yxmax,yymax;
    
    nelements = width * height;
    
	// minmax
	mini = USHRT_MAX;
	maxi = 0;
	for(i=0; i<nelements; i++){
		if ( mini > image[i] ) mini = image[i];
		if ( maxi < image[i] ) maxi = image[i];
	}
	printf("%d - %d\n",mini,maxi);
	
	threshold = (maxi/2 + mini/2);
	printf("%d\n",threshold);
	// search 
	xmaxcount = 0;
	for(y=0;y<height;y++){
		count = 0;
		for(x=0;x<width;x++){
			if (image[x+y*width] > threshold){
				//printf("->%d,%d\n",x,y);
				if (count == 0){
					xtmp = x; ytmp = y;
				}
				count++;
			} else {
				if (count > 0){
					if (count > xmaxcount){
						xmaxcount = count;
						xxmax = xtmp;
						xymax = ytmp;
					}
				}
				count = 0;
			}
		}
	}
	printf("x (%d,%d) - (%d)\n",xxmax,xymax,xmaxcount);
	*Y = xymax;
	
	// search 
	ymaxcount = 0;
	for(x=0;x<width;x++){
		count = 0;
		for(y=0;y<height;y++){
			if (image[x+y*width] > threshold){
				//printf("->%d,%d\n",x,y);
				if (count == 0){
					xtmp = x; ytmp = y;
				}
				count++;
			} else {
				if (count > 0){
					if (count > ymaxcount){
						ymaxcount = count;
						yxmax = xtmp;
						yymax = ytmp;
					}
				}
				count = 0;
			}
		}
	}
	printf("y (%d,%d) - (%d)\n",yxmax,yymax,ymaxcount);
	*X = yxmax;
	//printf("center = (%d,%d)\n",X,Y);
	// check
	if ((*X>xxmax) && (*X<(xxmax+xmaxcount)) && (*Y>yymax) && (*Y<(yymax + ymaxcount))) {
		printf("valid\n");
		return 0;	
	}
	return -1;
}

int main(int argc, char *argv[])
{
    fitsfile *fptr; 
    int status = 0;
    unsigned int bitpix, naxis;
    long naxes[2];
    long nelements;
    int anynul;
    unsigned short *image;
    int X,Y;
    int rc;
 
	if (!fits_open_file(&fptr, argv[1], READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
			nelements = naxes[0]*naxes[1];
			printf("%ld\n",nelements);
			image = malloc(sizeof(short)*nelements);
			
			if (bitpix == 16){
				fits_read_img(fptr,TUSHORT,1,nelements,NULL,image, &anynul, &status);
				rc = star16(image,naxes[0],naxes[1],&X,&Y);
				if (rc == 0)
					printf("center = (%d,%d)\n",X,Y);
			}
		}
	}
}
