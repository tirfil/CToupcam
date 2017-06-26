#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fitsio.h"
#include "toupcam.h"


#define BARRIER(x) rc = x; if(rc != 0){ printf("BARRIER %08x at line: %d\n",rc,__LINE__); exit(rc);}

#define SNAP_START 0
#define SNAP_STOP  1

typedef struct{
	unsigned int width;
	unsigned int height;
	unsigned int status;
	void* data;
} PushCxt;


int frameready;

void FitsWrite(unsigned char *raw, int width, int height, const char *filename){
    fitsfile *fptrout;
    int status = 0;
    long naxis=2;
    long naxes[2];
	unsigned long nelements;
	int index;
	
	nelements=width*height;
	
	naxes[0]=width;
	naxes[1]=height;
				
	remove(filename);
	fits_create_file(&fptrout,filename, &status);
	fits_create_img(fptrout, BYTE_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TBYTE, (long)1L, nelements, raw, &status);
	fits_close_file(fptrout, &status);
}

void FitsWrite16(unsigned char *raw, int width, int height, const char *filename){
    fitsfile *fptrout;
    int status = 0;
    long naxis=2;
    long naxes[2];
	unsigned long nelements;
	int index;
	unsigned short *im;
	
	nelements=width*height;
	naxes[0]=width;
	naxes[1]=height;
	
	im = malloc(sizeof(unsigned short)*nelements);
	
	for (index=0;index<nelements*2;index++){
		if (index%2 == 0){
			im[index/2] = (unsigned short)raw[index];
		} else {
			im[index/2] += (unsigned short)raw[index]*256;
		}
	}
				
	remove(filename);
	fits_create_file(&fptrout,filename, &status);
	fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TUSHORT, (long)1L, nelements, im, &status);
	fits_close_file(fptrout, &status);
	free(im);
}

void PullCallBack(unsigned int event, void* cx){
	printf("EventCallBack event=%d\n",event);
	switch(event){
		case TOUPCAM_EVENT_STILLIMAGE:
			frameready = 1;
			return;
	}

}

void PushCallBack(const void* pData, const BITMAPINFOHEADER* pHeader, BOOL bSnap, void* cx){
	unsigned int width;
	unsigned int height;
	PushCxt* cxt;
	cxt = (PushCxt*)cx;
	if (cxt->status == SNAP_STOP) return;
	width = cxt->width;
	height = cxt->height;
	if (pData != NULL && bSnap == TRUE){
		memcpy(cxt->data,pData,width*height*2);
		cxt->status = SNAP_STOP;
	}
	return;
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
	
	PushCxt pushcxt;
	
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
	
	BARRIER(Toupcam_put_Option(h,TOUPCAM_OPTION_BITDEPTH,1)); // 12 bits
	//BARRIER(Toupcam_put_Option(h,TOUPCAM_OPTION_RGB48,1));
	BARRIER(Toupcam_put_Option(h,TOUPCAM_OPTION_RAW,1)); // Raw mode
	
	BARRIER(Toupcam_put_AutoExpoEnable(h,0));
	BARRIER(Toupcam_put_ExpoTime(h, expo));
	BARRIER(Toupcam_put_ExpoAGain(h, gain));
	BARRIER(Toupcam_get_Size(h,&width, &height));

	
	BARRIER(Toupcam_put_eSize(h, 0));
	BARRIER(Toupcam_put_Speed(h,0));
	BARRIER(Toupcam_put_RealTime(h, 1));

#ifdef PullMode		
	raw = malloc(sizeof(unsigned char)*width*height*2);
	//raw = malloc(sizeof(unsigned char)*width*height);
	if (raw == NULL) {
			printf("Error: cannot allocate memory\n");
	}


	// clear cam buffer
	while(Toupcam_PullStillImage(h, raw, 24, &width, &height) == 0){}
	
	BARRIER(Toupcam_StartPullModeWithCallback(h,PullCallBack,NULL));
	
	Toupcam_Snap(h,0);
	
	start = time(NULL);
	
	threshold = 15 + (unsigned int)expo/1000000;
	
	while(1){
		if (frameready){
			BARRIER(Toupcam_PullStillImage(h,raw,24,&width,&height));
			printf("Capture Still Image: %d x %d\n",width,height);
			if (raw == NULL) {
				printf("Error: no data\n");
			}
			//FitsWrite(raw,2*width,height,"raw.fits");
			FitsWrite16(raw,width,height,"raw16.fits");
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
#else
	pushcxt.width = width;
	pushcxt.height = height;
	pushcxt.status = SNAP_START;
	raw = malloc(sizeof(unsigned char)*width*height*2);
	pushcxt.data = raw;
	BARRIER(Toupcam_StartPushMode(h,PushCallBack,&pushcxt));
	
	start = time(NULL);
	
	threshold = 15 + (unsigned int)expo/1000000;
	
	Toupcam_Snap(h,0);
	
	while(1){
		if (pushcxt.status == SNAP_STOP){
			printf("Capture Still Image: %d x %d\n",width,height);
			FitsWrite16(raw,width,height,"raw16.fits");
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
#endif
}
