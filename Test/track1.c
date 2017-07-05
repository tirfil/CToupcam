#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
//#include "fitsio.h"
#include "toupcam.h"


#define BARRIER(x) rc = x; if(rc != 0){ printf("BARRIER %08x at line: %d\n",rc,__LINE__); exit(rc);}
#define DELAYSEC 1


typedef struct {
	int x;
	int y;
	int step;
	int mode;
} track_info;


int frameready;

int algo(HToupCam h, int x, int y, track_info* info){
int dx,dy;
int factor;
int dir;
int mode;
int rc; // for BARRIER

	if (info->step == 0){
		factor = 1;
		info->x = x;
		info->y = y;
		info->step = 1;
		printf("init\n");
		return 0;
	}
	if (info->step < 5){
		if ((info->x == x) && (info->y == y)) {
			info->step++;
			return 0;
		}
		if ((abs(info->x-x) > 1) || (abs(info->y-y) > 1)){
			info->step = 0;
			return 0;
		}
		info->x = x;
		info->y = y;
		info->step++;
		return 0;
	}
	if (info->step == 6){
		printf("calibration factor is %d\n", factor);
		info->x = x;
		info->y = y;
		// Check North Move	
		BARRIER(Toupcam_ST4PlusGuide(h,0,factor));
		sleep(DELAYSEC);
		info->step= 7;
		return 0;
	}
	if (info->step == 7){
		dx = x - info->x;
		dy = y - info->y;
		info->x = x;
		info->y = y;
		if (dx==0) {
			if (dy>1){
				info->mode = 0;
				info->step = 8;
				printf("mode is %d\n",info->mode);
				return 0;
			}
			if (dy<-1){ 
				info->mode = 1;
				info->step = 8;
				printf("mode is %d\n",info->mode);
				return 0;
			}
			factor = factor * 2;
			info->step = 6;
			return 0;
		}
		if (dy==0) {
			if (dx>1){
				info->mode = 2;
				info->step = 8;
				printf("mode is %d\n",info->mode);
				return 0;
			}
			if (dx<-1){ 
				info->mode = 3;
				info->step = 8;
				printf("mode is %d\n",info->mode);
				return 0;
			}
			factor = factor * 2;
			info->step = 6;
			return 0;
		}
	}
	if (info->step == 8){
		mode = info->mode;
		dx = x - info->x;
		dy = y - info->y;
		if (dx != 0) {
			if (dx>0){
				if (mode == 0) dir = 3;
				if (mode == 1) dir = 2;
				if (mode == 2) dir = 1;
				if (mode == 3) dir = 0;
			}
			if (dx<0){
				if (mode == 0) dir = 2;
				if (mode == 1) dir = 3;
				if (mode == 2) dir = 0;
				if (mode == 3) dir = 1;			
			}
			printf("dir: %d\n",dir);
			BARRIER(Toupcam_ST4PlusGuide(h,dir,factor/2));
			sleep(DELAYSEC);
			info->step = 9;
			return 0;
		}
		if (dy != 0) {		
			if (dy>0){
				if (mode == 0) dir = 1;
				if (mode == 1) dir = 0;
				if (mode == 2) dir = 3;
				if (mode == 3) dir = 2;
			}
			if (dy<0){
				if (mode == 0) dir = 0;
				if (mode == 1) dir = 1;
				if (mode == 2) dir = 2;
				if (mode == 3) dir = 3;			
			}
			printf("dir: %d\n",dir);
			BARRIER(Toupcam_ST4PlusGuide(h,dir,factor/2));
			sleep(DELAYSEC);
			info->step = 10;
			return 0;
		}
		// no change
		return 0;
	}
	if (info->step == 9){
		info->x = x;
		info->step = 8;
		return 0;
	}
	if (info->step == 10){
		info->y = y;
		info->step = 8;
		return 0;
	}
}

int star8(unsigned char *image, unsigned int width, unsigned int height, int* X, int* Y)
{
    long nelements;	
	int x,y;
    int i;
    unsigned char mini, maxi;
    unsigned char threshold;
    int count, xmaxcount,ymaxcount;
    int xtmp,ytmp,xxmax,xymax,yxmax,yymax;
    
    nelements = width * height;
    
	// minmax
	mini = UCHAR_MAX;
	maxi = 0;
	for(i=0; i<nelements; i++){
		if ( mini > image[i] ) mini = image[i];
		if ( maxi < image[i] ) maxi = image[i];
	}
	printf("%d - %d\n",mini,maxi);
	
	threshold = (maxi/2 + mini/2);
	//printf("%d\n",threshold);
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

/*
void Stat(unsigned char *raw, int width, int height){
	long nelements;
	int index;
	unsigned char min;
	unsigned char max;
	unsigned char mono;
	
	min = UCHAR_MAX;
	max = 0;

	nelements=width*height;
	for(index=0;index<nelements;index++){
		mono = (unsigned char)raw[index];
		if (mono < min) min = mono;
		if (mono > max) max = mono;
	}
	printf("\tMin\tMax\n");
	printf("\t---\t---\n");
	printf("mono\t%d\t%d\n\n",min, max);
}

void FitsWrite(unsigned char *raw, int width, int height, const char *filename, unsigned int* expo, unsigned short* gain){
    fitsfile *fptrout;
    int status = 0;
    long naxis=2;
    long naxes[2];
	unsigned long nelements;
	int index;
	time_t current_time;
	char* c_time_string;
	
	current_time = time(NULL);
	c_time_string = ctime(&current_time);
		
	nelements=width*height;
	
	naxes[0]=width;
	naxes[1]=height;
				
	remove(filename);
	fits_create_file(&fptrout,filename, &status);
	fits_create_img(fptrout, BYTE_IMG, naxis, naxes, &status);
	fits_update_key(fptrout, TSTRING, "DEVICE","Touptek CAM","CCD device name",&status);
	fits_update_key(fptrout, TUINT, "EXPOSURE", expo, "Total Exposure Time (us)", &status);
	fits_update_key(fptrout, TUSHORT, "GAIN", gain, "CCD gain (x100)", &status);
	fits_update_key(fptrout, TSTRING, "DATE", c_time_string, "Date & time", &status);
	fits_write_img(fptrout, TBYTE, (long)1L, nelements, raw, &status);
	fits_close_file(fptrout, &status);
}
*/

void EventCallBack(unsigned int event, void* cx){
	//printf("EventCallBack event=%d\n",event);
	switch(event){
		case TOUPCAM_EVENT_STILLIMAGE:
			frameready = 1;
			return;
	}

}

int main(int argc, char* argv[])
{
	unsigned int cnt;
	ToupcamInst arr[TOUPCAM_MAX];
	const ToupcamModel *model;
	int i;
	int rc;
	unsigned short gain_min, gain_max, definition;
	HToupCam h;
	unsigned int width;
	unsigned int height;
	unsigned char *raw;
	time_t start,current;
	unsigned int expo;
	unsigned short gain;
	unsigned int delta,threshold;
	int noerror;
	int x,y;
	track_info info;
	
	printf("Library version: %s\n",Toupcam_Version());
	
	frameready = 0;
	
	
	cnt = Toupcam_Enum(arr);
	if (cnt == 0) {
		printf("No ToupCam found!!\n");
		exit(0);
	}

	model = arr[0].model;
	printf("Model found  %s\n",model->name);
	printf("Capabilities 0x%08x\n",model->flag);
	
	if (argc != 3) {
		printf("Usage: %s expo_time_ms gain_x_100\n",argv[0]);
		exit(0);
	}
	
	expo = atoi(argv[1]); // ms
	expo = expo*1000; // us
	gain = atoi(argv[2]);
	
	h = Toupcam_Open(NULL); // open first camera
	if (h == NULL){
		printf("Cannot open ToupCam\n");
		exit(0);
	}
	
	//printf("Still resolution: %d\n",Toupcam_get_StillResolutionNumber(h));
	//printf("Max bit depth: %d\n",Toupcam_get_MaxBitDepth(h));
	BARRIER(Toupcam_get_ExpoAGainRange(h, &gain_min, &gain_max, &definition));
	printf("Gain range: from %d to %d. Default is %d\n",gain_min,gain_max,definition);
	if (gain > gain_max) {
		gain = gain_max;
		printf("Set gain to %d\n",gain);
	}
	if ( gain < gain_min) {
		gain = gain_min;
		printf("Set gain to %d\n",gain);
	}
	
	printf("Exposure 	is %d uS\n",expo);
	printf("Gain (x100) is %d \n",gain);
	
	BARRIER(Toupcam_put_Option(h,TOUPCAM_OPTION_BITDEPTH,0)); // 8 bits
	//BARRIER(Toupcam_put_Option(h,TOUPCAM_OPTION_RGB48,1));
	BARRIER(Toupcam_put_Option(h,TOUPCAM_OPTION_RAW,0)); // RGB mode
	
	BARRIER(Toupcam_put_AutoExpoEnable(h,0));
	BARRIER(Toupcam_put_ExpoTime(h, expo));
	BARRIER(Toupcam_put_ExpoAGain(h, gain));

	
	BARRIER(Toupcam_put_eSize(h, 0));
	BARRIER(Toupcam_put_Speed(h,0));
	BARRIER(Toupcam_put_RealTime(h, 1));
	BARRIER(Toupcam_get_Size(h,&width, &height));
	
	raw = malloc(sizeof(unsigned char)*width*height);
	if (raw == NULL) {
			printf("Error: cannot allocate memory\n");
	}
	
	// clear cam buffer
	while(Toupcam_PullStillImage(h, raw, 8, &width, &height) == 0){}
	
	BARRIER(Toupcam_StartPullModeWithCallback(h,EventCallBack,NULL));
	
	noerror = 1;
	
	info.step = 0;
	
	while(noerror) {
	
		Toupcam_Snap(h,0);
		
		start = time(NULL);
		
		threshold = 15 + (unsigned int)expo/1000000;
		
		while(1){
			if (frameready){
				BARRIER(Toupcam_PullStillImage(h,raw,8,&width,&height));
				printf("Capture Still Image: %d x %d\n",width,height);
				if (raw == NULL) {
					printf("Error: no data\n");
					noerror = 0;
				}
				//Stat(raw,width,height);
				//FitsWrite(raw,width,height,"mono.fits",&expo,&gain);
				rc = star8(raw,width,height,&x, &y);
				if (rc == 0){
					printf("center = (%d,%d)\n",x,y);
					algo(h,x,y,&info);
				}
				frameready = 0;
				//Toupcam_Stop(h);
				break;
			}
			current = time(NULL);
			delta = (unsigned int)difftime(current,start);
			if ( delta > threshold ){
				printf("Timeout expires\n");
				noerror = 0;
				break;
			}
		}
		
	}
	Toupcam_Stop(h);
	Toupcam_Close(h);
	free(raw);
}
