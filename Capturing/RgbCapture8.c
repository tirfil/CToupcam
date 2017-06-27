#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fitsio.h"
#include "toupcam.h"


#define BARRIER(x) rc = x; if(rc != 0){ printf("BARRIER %08x at line: %d\n",rc,__LINE__); exit(rc);}


int frameready;

void Stat3D(unsigned char *raw, int width, int height){
	long nelements;
	int index;
	unsigned char minred, mingreen, minblue;
	unsigned char maxred, maxgreen, maxblue;
	unsigned char red,green,blue;
	
	minred = UCHAR_MAX;
	mingreen = UCHAR_MAX;
	minblue = UCHAR_MAX;
	maxred = 0;
	maxgreen = 0;
	maxblue = 0;
	nelements=width*height;
	for(index=0;index<nelements;index++){
		red = (unsigned char)raw[4*index];
		green = (unsigned char)raw[4*index+1];
		blue = (unsigned char)raw[4*index+2];
		if (red < minred) minred = red;
		if (green < mingreen) mingreen = green;
		if (blue < minblue) minblue = blue;
		if (red > maxred) maxred = red;
		if (green > maxgreen) maxgreen = green;
		if (blue > maxblue) maxblue = blue;
	}
	printf("\tMin\tMax\n");
	printf("\t---\t---\n");
	printf("red\t%d\t%d\n",minred, maxred);
	printf("green\t%d\t%d\n",mingreen, maxgreen);
	printf("blue\t%d\t%d\n\n",minblue,maxblue);
}

void Fits3DWrite(unsigned char *raw, int width, int height, const char *filename, unsigned int* expo, unsigned short* gain){
    fitsfile *fptrout;
    int status = 0;
    unsigned int naxis=3;
    unsigned int naxis3=2;
    long naxes3[3];
	long nelements;
	int wcsaxes= 2;
	unsigned char *red;
    unsigned char *green;
	unsigned char *blue;
	int index;
	time_t current_time;
	char* c_time_string;
	
	current_time = time(NULL);
	c_time_string = ctime(&current_time);
	
	nelements=width*height;
	
	naxes3[0]=width;
	naxes3[1]=height;
	naxes3[2]=3;
				
	red = malloc(sizeof(unsigned char)*nelements);
	green = malloc(sizeof(unsigned char)*nelements);
	blue = malloc(sizeof(unsigned char)*nelements);
	
	for(index=0;index<nelements;index++){
		red[index] = (unsigned char)raw[4*index];
		green[index] = (unsigned char)raw[4*index+1];
		blue[index] = (unsigned char)raw[4*index+2];
	}
	remove(filename);
	fits_create_file(&fptrout,filename, &status);
	fits_create_img(fptrout, BYTE_IMG, naxis, naxes3, &status);
	fits_update_key(fptrout, TSTRING, "DEVICE","Touptek CAM","CCD device name",&status);
	fits_update_key(fptrout, TUINT, "EXPOSURE", expo, "Total Exposure Time (us)", &status);
	fits_update_key(fptrout, TUSHORT, "GAIN", gain, "CCD gain (x100)", &status);
	fits_update_key(fptrout, TSTRING, "DATE", c_time_string, "Date & time", &status);
	fits_update_key(fptrout, TSTRING, "CSPACE","sRGB","",&status);
	fits_update_key(fptrout, TINT, "WCSAXES", &wcsaxes, "", &status);
	fits_write_img(fptrout, TBYTE, 1, nelements, red, &status);
	fits_write_img(fptrout, TBYTE, nelements+1, nelements, green, &status);
	fits_write_img(fptrout, TBYTE, 2*nelements+1, nelements, blue, &status);
	fits_close_file(fptrout, &status);
	free(red);
	free(green);
	free(blue);
}

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
	
	raw = malloc(sizeof(unsigned char)*width*height*4);
	if (raw == NULL) {
			printf("Error: cannot allocate memory\n");
	}
	
	// clear cam buffer
	while(Toupcam_PullStillImage(h, raw, 32, &width, &height) == 0){}
	
	BARRIER(Toupcam_StartPullModeWithCallback(h,EventCallBack,NULL));
	
	Toupcam_Snap(h,0);
	
	start = time(NULL);
	
	threshold = 15 + (unsigned int)expo/1000000;
	
	while(1){
		if (frameready){
			BARRIER(Toupcam_PullStillImage(h,raw,32,&width,&height));
			printf("Capture Still Image: %d x %d\n",width,height);
			if (raw == NULL) {
				printf("Error: no data\n");
			}
			Stat3D(raw,width,height);
			Fits3DWrite(raw,width,height,"rgb32.fits",&expo,&gain);
			frameready = 0;
			Toupcam_Stop(h);
			break;
		}
		current = time(NULL);
		delta = (unsigned int)difftime(current,start);
		if ( delta > threshold ){
			printf("Timeout expires\n");
			Toupcam_Stop(h);
			break;
		}
	}
	Toupcam_Close(h);
	free(raw);
}
