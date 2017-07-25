#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "libusb.h"
#include "fitsio.h"
/*
 * 
 * https://github.com/AnsonLuo/e100_sdk_v1.2
 * 
    IMX290_STANDBY                  = 0x3000,
    IMX290_MASTERSTOP               = 0x3002,
    IMX290_RESET                    = 0x3003,
    IMX290_AGAIN                    = 0x3014,
    IMX290_VMAX_LSB                 = 0x3018,
    IMX290_VMAX_MSB                 = 0x3019,
    IMX290_VMAX_HSB                 = 0x301A,
    IMX290_SHS1_LSB                 = 0x3020,
    IMX290_SHS1_MSB                 = 0x3021,
    IMX290_SHS1_HSB                 = 0x3022,
    IMX290_SHS2_LSB                 = 0x3024,
    IMX290_SHS2_MSB                 = 0x3025,
    IMX290_SHS2_HSB                 = 0x3026,
    IMX290_RHS1_LSB                 = 0x3030,
    IMX290_RHS1_MSB                 = 0x3031,
    IMX290_RHS1_HSB                 = 0x3032,
    IMX290_DOL_FORMAT               = 0x3045,
    IMX290_DOL_SYNCSIGNAL           = 0x3106,
    IMX290_DOL_HBFIXEN              = 0x3107,
	IMX290_NULL0_SIZEV 				= 0x3415,
*/

#define BARRIER(x) rc = x; if(rc < 0){ printf("BARRIER %08x at line: %d\n",rc,__LINE__); exit(rc);}


static struct libusb_device_handle *handle = NULL;
static int seq = 1;
static unsigned int VID = 0x0547;
static unsigned int PID = 0x10FE;

static int done;

static unsigned short key;
static unsigned short keyrot;

/**
 * Find Device
 **/
static int rc_find_device() {
	handle = libusb_open_device_with_vid_pid(NULL, VID, PID);
	return handle ? 0 : -EIO;
}

/**
 * Open
 **/
static int rc_open() {
	int r;
	r = libusb_init(NULL);
	if (r < 0) return r;

	// debug	
	libusb_set_debug(NULL,3);

	// search
	r = rc_find_device();
	if (r < 0) {
		fprintf(stderr, "rc_open error: unable find device\n");
		return r;
	}
	
	// claim
	r = libusb_claim_interface(handle, 0);
	if (r < 0) {
		fprintf(stderr, "rc_open error: unable claim interface\n");
		return r;	
	}
	
	//fprintf(stderr,"rc_open\n");
	return 0;
}

/**
 * Close
 **/
static int rc_close() {
	// close
	libusb_close(handle);
	// exit	
	libusb_exit(NULL);
	//fprintf(stderr,"rc_close\n");
}

/**
 * Write buffer to RAW file
 **/
void rc_write(unsigned char *img, int len, const char *basename)
{
	FILE *fo;
	fo = fopen (basename, "wb");
	fwrite(img, len, sizeof(char), fo);
	fclose(fo);
}

void FitsWrite16(unsigned char *raw, int width, int height, const char *filename){
    fitsfile *fptrout;
    int status = 0;
    long naxis=2;
    long naxes[2];
	unsigned long nelements;
	int index;
	unsigned short *im;
	time_t current_time;
	char* c_time_string;
	unsigned short min;
	unsigned short max;
	unsigned short value;

	
	min = USHRT_MAX;
	max = 0;
	
	current_time = time(NULL);
	c_time_string = ctime(&current_time);
	
	nelements=width*height;
	naxes[0]=width;
	naxes[1]=height;
	
	im = malloc(sizeof(unsigned short)*nelements);
	
	for (index=0;index<nelements*2;index++){
		if (index%2 == 0){
			im[index/2] = (unsigned short)raw[index];
		} else {
			im[index/2] += (unsigned short)raw[index]*256;
			value = im[index/2];
			if (value < min) min = value;
			if (value > max) max = value;
			
		}
	}
	
	printf("\tMin\tMax\n");
	printf("\t---\t---\n");
	printf("Bayer\t%d\t%d\n\n",min, max);
	
				
	remove(filename);
	fits_create_file(&fptrout,filename, &status);
	fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
	fits_update_key(fptrout, TSTRING, "DEVICE","Touptek CAM","CCD device name",&status);
	fits_update_key(fptrout, TSTRING, "DATE", c_time_string, "Date & time", &status);
	fits_write_img(fptrout, TUSHORT, (long)1L, nelements, im, &status);
	fits_close_file(fptrout, &status);
	free(im);
}

//void mycallback(struct libusb_transfer *xfr)
//{
    //printf("mycallback\n");
    //switch(xfr->status)
    //{
        //case LIBUSB_TRANSFER_COMPLETED:
            //// Success here, data transfered are inside 
            //// xfr->buffer
            //// and the length is
            //// xfr->actual_length
            //done = 1;
            //break;
        //case LIBUSB_TRANSFER_CANCELLED:
        //case LIBUSB_TRANSFER_NO_DEVICE:
        //case LIBUSB_TRANSFER_TIMED_OUT:
        //case LIBUSB_TRANSFER_ERROR:
        //case LIBUSB_TRANSFER_STALL:
        //case LIBUSB_TRANSFER_OVERFLOW:
            //// Various type of errors here
            //printf("xfr status= %d\n",xfr->status);
            //done = 3;
            //break;
    //}
//}

int write_register_b(unsigned short address,unsigned short data){
	unsigned short cadd;
	unsigned short cdat;
	char dummy;
	address = address ^ keyrot;
	data = data ^ keyrot;
	return libusb_control_transfer(handle,0xc0,0x0b,data,address,&dummy,1,500);
}

int write_register_d(unsigned short address,unsigned short data){
	unsigned short cadd;
	unsigned short cdat;
	char dummy;
	address = address ^ keyrot;
	data = data ^ keyrot;
	return libusb_control_transfer(handle,0xc0,0x0d,data,address,&dummy,1,500);
}


main(int argc, char* argv[]){
	
	unsigned char buffer[4147200] = {};
	unsigned int timeout;
	unsigned int transferred;
	unsigned int rc;
	unsigned char challenge[] = {0xf2, 0xed, 0x70, 0xc6, 0xb6, 0xd9, 0x26, 0xb4, 0x71, 0xdf, 0xbb, 0xbc, 0x8c, 0x34, 0xfd, 0xbf};
	
	struct libusb_transfer *xfr;
	unsigned char *data;
	unsigned int size;
	
	int i;
	
	unsigned short gain;
	unsigned short gain_reg;
	
	unsigned int expo_ms;
	unsigned int temp;
	unsigned int shs1;
	unsigned short reg5000;
	unsigned short reg4000;
	
	done = 0;

	//timeout = 500;
	timeout = 0;
	
	gain = 100;
	expo_ms = 1000;
	
	if (argc != 3) {
		printf("Usage: %s expo_time_ms gain_x_100\n",argv[0]);
		exit(0);
	}
	
	expo_ms = atoi(argv[1]); // ms
	gain = atoi(argv[2]);
	
	
	if (gain > 5000) gain = 5000;
	if (gain < 100) gain = 100;
	
	gain_reg = (int)(20.0*log((double)gain/100.0)/log(2.0));
	
	printf("gain_reg= 0x%04x\n",gain_reg);
	
	/*
	UINT32 SHS1 = IMX290Ctrl.Status.ModeInfo.NumExposureStepPerFrame - NumXhsEshrSpeed;
	IMX290_RegRW(IMX290_SHS1_LSB, (SHS1 & 0xff)); 
    IMX290_RegRW(IMX290_SHS1_MSB, (SHS1 >> 8) & 0xff);
	IMX290_RegRW(IMX290_SHS1_HSB, (SHS1 >> 16) & 0x03);
	*/
	
	/* IMX290Ctrl.Status.ModeInfo.NumExposureStepPerFrame = @ 0x5000 */
	
	/* NumXhsEshrSpeed = 4.89 per ms */
	
	temp = (int)ceil((double)expo_ms*4.89);
	
	if (temp > 1024 ){
		shs1 = 7;
		temp = temp + 7;
		reg5000 = temp & 0xffff;
		reg4000 = (temp >> 16) & 0xffff;
	} else {
		reg5000 = 1125; // 0x465
		shs1 = 1125 - temp;
		reg4000 = 0;
	}
	
	printf("shs1= 0x%06x\n",shs1);
	printf("reg5000= 0x%04x\n",reg5000);
	printf("reg4000= 0x%04x\n",reg4000);
	
	//key = 0x0000;
	key = 0x41f2;	
	
	keyrot = ((key << 12) & 0xffff) + (key >> 4);
	
	if (rc_open() != 0) {
		printf("Open failed\n");
		exit(-1);
	}
	
	BARRIER(libusb_control_transfer(handle,0xC0,0x16,key,0x0000,buffer,2,timeout)); // crypt key set at 0x0000
	//printf("%d\n",buffer[0]);
	buffer[0] = 0;
	
	// f2ed70c6 b6d926b4 71dfbbbc 8c34fdbf -> 3c57bc13 05a8ee00 3845fc01 cf96e204 (41f2)
	// d51ebe7f 0b314ce2 65b4c39f 2b54b37c -> a16b0cc9 d7fe9a31 a97906e1 ef147ac3 (bfd5)
	// 8468cace aeff16b3 f2c10d18 c6848d62 -> ccb11439 fa4c6022 3622535b 2ac9efa9 (6684)
	// 0078437b 831382e8 80821af8 835ea95c -> f26bb3cc 79ea56bd fa7d12f1 81dda5b9 (a600)
	// 908e627a 966036d1 4559af48 f7011003 -> a2a5968f ac774ae2 6374e761 11404c20 (9990)
	// 2c8118f3 ab81798a da04b775 1d96713c -> 3f942823 e1b88d9f 151ef0ad 5bb48d59 (222c)
	// 0a33b356 7993af0b f4994720 797fa9e7 -> 1e64ca88 8aa4e23e 10b6625e 92bce806 (a20a)
	// 969b2459 f98b3dd3 2518200e 0c86d3e6 -> 7373fe37 d5e398b1 756df6e5 dddbaa3d (6696)
	// 5ca3c672 81648a9f e673c588 45204207 -> deaa4673 87e78a24 f4825191 4f2f4a14 (cc5c)
	// ff5462cc 5c2ee391 61c664d9 b048bbd3 -> 41b7a20d c29547d6 cb11ac42 fe970720 (3fff)
	// abe57dc9 2690fd6f 85bcb05f dc3a609c -> 7eb74d1a fce65143 60970938 3a18bdf9 (bbab)
	// 77b28766 2d44e9fc 7f04adac 251f4064 -> 41015333 7b8fb1c9 41cb7171 6be680a9 (6777)
	// ee135212 511be775 c396df42 8176f743 -> 476bad6c ae7746d4 13e73194 d6cb4e9a (feee)
	// 662afcf7 a79a4ea1 b6b823b4 0e9e21a8 -> cc913c58 0de18ee6 04038b1d 5ce989f5 (6066)
	// 5532ad62 b5add17f 572c788e e87897eb -> 9dfbf7ad 81fa1f4e 97edbad1 2cbd5d32 (6555)
	// bb5728e3 8672915d 0918c10d 57368514 -> bc572be6 8b769863 1221cb18 63429423 (b9bb)
	// 276da5ce 820f6a9c 8a98489b 9a922326 -> 2d70a5d3 88166aa1 94a754a8 a89d2b33 (2727)
	// 4de1c025 c0f83fe4 116d6354 aa571ea9 -> 4de1c025 c0f83fe4 116d6354 aa571ea9 (444d)
	// ca3bda46 898f1314 0d00d443 7dbead6b -> e856f762 a7ae4b50 2032e954 8fd4c180 (caca)
	BARRIER(libusb_control_transfer(handle,0x40,0x4a,0x0000,0x0000,challenge,16,timeout));  
	BARRIER(libusb_control_transfer(handle,0xC0,0x6a,0x0000,0x0000,buffer,16,timeout));
	//for (i=0;i<16;i++){
		//printf("%02x",buffer[i]);
	//}
	//printf("\n");
	BARRIER(libusb_control_transfer(handle,0xC0,0x20,0x0000,0x0000,buffer,4,timeout));
	//for (i=0;i<4;i++){
		//printf("%02x",buffer[i]);
	//}
	//printf("\n");
	BARRIER(libusb_control_transfer(handle,0xC0,0x20,0x0000,0x0000,buffer,923,timeout));
	/*for (i=0;i<923;i++){
		printf("%02x",buffer[i]);
	}
	printf("\n");*/	
	BARRIER(libusb_control_transfer(handle,0xC0,0x0a,0x0000,0x301e,buffer,3,timeout));
	
	BARRIER(write_register_b(0x3003,0x0001)); // IMX290_RESET
	BARRIER(write_register_b(0x305c,0x0018));
	BARRIER(write_register_b(0x305d,0x0000));
	BARRIER(write_register_b(0x305e,0x0020));
	BARRIER(write_register_b(0x305f,0x0001));
	
	BARRIER(write_register_b(0x315e,0x001a));
	BARRIER(write_register_b(0x3164,0x001a));
	BARRIER(write_register_b(0x3048,0x0049)); 
	BARRIER(write_register_b(0x300f,0x0000));
	BARRIER(write_register_b(0x3010,0x0021));	
	
	BARRIER(write_register_b(0x3012,0x0064));
	BARRIER(write_register_b(0x3016,0x0009));
	BARRIER(write_register_b(0x3070,0x0002));
	BARRIER(write_register_b(0x3071,0x0011));
	BARRIER(write_register_b(0x309b,0x0010));		
	
	BARRIER(write_register_b(0x309c,0x0022));
	BARRIER(write_register_b(0x30a2,0x0002));
	BARRIER(write_register_b(0x30a6,0x0020));
	BARRIER(write_register_b(0x30a8,0x0020));
	BARRIER(write_register_b(0x30aa,0x0020));	
	
	BARRIER(write_register_b(0x30ac,0x0020));
	BARRIER(write_register_b(0x30b0,0x0043));
	BARRIER(write_register_b(0x3119,0x009e));
	BARRIER(write_register_b(0x311c,0x001e));
	BARRIER(write_register_b(0x311e,0x0008));	
	
	BARRIER(write_register_b(0x3128,0x0005));
	BARRIER(write_register_b(0x313d,0x0083));
	BARRIER(write_register_b(0x3150,0x0003));
	BARRIER(write_register_b(0x317e,0x0000));
	BARRIER(write_register_b(0x32b8,0x0050));		
	
	BARRIER(write_register_b(0x32b9,0x0010));
	BARRIER(write_register_b(0x32ba,0x0000));
	BARRIER(write_register_b(0x32bb,0x0004));
	BARRIER(write_register_b(0x32c8,0x0050));
	BARRIER(write_register_b(0x32c9,0x0010));			
	
	BARRIER(write_register_b(0x32ca,0x0000));
	BARRIER(write_register_b(0x32cb,0x0004));
	BARRIER(write_register_b(0x332c,0x00d3));
	BARRIER(write_register_b(0x332d,0x0010));
	BARRIER(write_register_b(0x332e,0x000d));				
	
	BARRIER(write_register_b(0x3358,0x0006));
	BARRIER(write_register_b(0x3359,0x00e1));
	BARRIER(write_register_b(0x335a,0x0011));
	BARRIER(write_register_b(0x3360,0x001e));
	BARRIER(write_register_b(0x3361,0x0061));		

	BARRIER(write_register_b(0x3362,0x0010));
	BARRIER(write_register_b(0x33b0,0x0050));
	BARRIER(write_register_b(0x33b2,0x001a));
	BARRIER(write_register_b(0x33b3,0x0004));
	BARRIER(write_register_b(0x3005,0x0001));			
	
	BARRIER(write_register_b(0x3046,0x0001));
	BARRIER(write_register_b(0x3007,0x0040));
	BARRIER(write_register_b(0x3009,0x0012));
	BARRIER(write_register_b(0x3012,0x0064));
	BARRIER(write_register_b(0x3013,0x0000));		
	
	BARRIER(write_register_b(0x300a,0x0000));
	BARRIER(write_register_b(0x300b,0x0000));
	BARRIER(write_register_b(0x3129,0x0000));
	BARRIER(write_register_b(0x317c,0x0000));
	BARRIER(write_register_b(0x31ec,0x000e));	
	
	BARRIER(write_register_d(0x0200,0x0001)); //
	
	BARRIER(write_register_b(0x303c,0x0008));
	BARRIER(write_register_b(0x303d,0x0000));
	BARRIER(write_register_b(0x303e,0x0038));
	BARRIER(write_register_b(0x303f,0x0004));
	BARRIER(write_register_b(0x3040,0x000c));		
	
	BARRIER(write_register_b(0x3041,0x0000));
	BARRIER(write_register_b(0x3042,0x0080));
	BARRIER(write_register_b(0x3043,0x0007));	
	
	BARRIER(write_register_d(0x8200,0x0780));
	BARRIER(write_register_d(0x8400,0x0438));
	BARRIER(write_register_d(0x8600,0x0000));
	BARRIER(write_register_d(0x8800,0x000d));
	BARRIER(write_register_d(0x8000,0x3520));	//
	
	BARRIER(write_register_b(0x3001,0x0001));
	BARRIER(write_register_b(0x3020,0x0007)); //IMX290_SHS1_LSB
	BARRIER(write_register_b(0x3021,0x0000)); //IMX290_SHS1_MSB	
	BARRIER(write_register_b(0x3022,0x0000)); //IMX290_SHS1_HSB
		
	BARRIER(write_register_d(0x4000,0x0000));
	BARRIER(write_register_d(0x5000,0x04eb)); // change with gain/exposure ?

	BARRIER(write_register_b(0x3001,0x0000));
	BARRIER(write_register_b(0x3000,0x0000)); //IMX290_STANDBY
	BARRIER(write_register_b(0x3001,0x0001));
	//BARRIER(write_register_b(0x3020,0x0034)); //IMX290_SHS1_LSB
	//BARRIER(write_register_b(0x3021,0x0004)); //IMX290_SHS1_MSB
	//BARRIER(write_register_b(0x3022,0x0000)); //IMX290_SHS1_HSB
	BARRIER(write_register_b(0x3020,(shs1 & 0xff))); 		//IMX290_SHS1_LSB
	BARRIER(write_register_b(0x3021,(shs1 >> 8) & 0xff)); 	//IMX290_SHS1_MSB
	BARRIER(write_register_b(0x3022,(shs1 >> 16) & 0x03)); 	//IMX290_SHS1_HSB
	
	//BARRIER(write_register_d(0x4000,0x0000));
	//BARRIER(write_register_d(0x5000,0x0465));
	BARRIER(write_register_d(0x4000,reg4000));
	BARRIER(write_register_d(0x5000,reg5000));
	BARRIER(write_register_b(0x3001,0x0000)); 
	BARRIER(write_register_d(0x0a00,0x0000));
	BARRIER(write_register_d(0x0a00,0xffff));	
	
	BARRIER(write_register_d(0x0a00,0x0000));
	BARRIER(write_register_d(0x0a00,0x0000));
	BARRIER(write_register_d(0x0a00,0x0000));
	BARRIER(write_register_d(0x0a00,0xffff));
		
	BARRIER(write_register_b(0x3001,0x0001));
	BARRIER(write_register_b(0x3020,0x0007));
	BARRIER(write_register_b(0x3021,0x0000));
	BARRIER(write_register_b(0x3022,0x0000));
	
	BARRIER(write_register_d(0x4000,0x0000));
	BARRIER(write_register_d(0x5000,0x04eb)); // change with gain/exposure ?
	BARRIER(write_register_b(0x3001,0x0000));
	//BARRIER(write_register_b(0x3014,0x0000));	
	BARRIER(write_register_b(0x3014,gain_reg));	// IMX290_AGAIN
	
	//sleep(expo_ms/1000+1);
	
	BARRIER(libusb_control_transfer(handle,0x40,0x01,0x0003,0x000F,buffer,0,timeout)); 
	
	//sleep(expo_ms/1000+2);
	usleep(expo_ms+1000);
	
	size = 0;
	do {
		BARRIER(libusb_bulk_transfer(handle,0x82,buffer+size,0x20000,&transferred,0));
		size += transferred;
		printf("transferred=%d total=%d\n",transferred,size);
	} while(transferred==0x20000);
	//} while(size != 4147200);
	//} while(size < 4100000);
	if (size)
		//rc_write(buffer,size,"raw.dat");
		printf("Capturing %d bytes\n\n",size);
		FitsWrite16(buffer,1920,1080,"usb.fits");
		//FitsWrite16(buffer,1936,1096,"usb.fits");
	
	//
/*	
	// 1920 x 1080 = 2073600
	// X 2 = 4147200
	//size = 4147200;
	size = 131072;
	//size = 1000;
	data = malloc(size);
	xfr = libusb_alloc_transfer(0);
    printf("libusb_alloc_transfer\n");
	libusb_fill_bulk_transfer(xfr,
                          handle,
                          0x82, // Endpoint ID
                          data,
                          size,
                          mycallback,
                          NULL,
                          5000
                          );
	printf("libusb_fill_bulk_transfer\n");
	if(libusb_submit_transfer(xfr) < 0)
	{
    // Error
		done = 2;
	}
	printf("libusb_submit_transfer\n");
	while (1){
		if (done == 1){
			printf("size = %d\n",size);
			printf("length= %d\n",xfr->actual_length);
			rc_write(xfr->buffer,xfr->actual_length,"raw.dat");
			break;
		}
		if (done == 2){
			printf("libusb_submit_transfer error\n");
			break;
		}
		if (done == 3){
			printf("mycallback error\n");
			break;
		}
	}
	
	libusb_free_transfer(xfr);
	free(data);
	*/

	BARRIER(write_register_d(0x0a00,0x0000));
	BARRIER(write_register_d(0x0a00,0x0000));
	BARRIER(write_register_b(0x3003,0x0001)); //IMX290_RESET
	
	BARRIER(libusb_control_transfer(handle,0x40,0x01,0x0000,0x000F,buffer,0,timeout));
	BARRIER(libusb_control_transfer(handle,0xC0,0x17,0x0000,0x0000,buffer,2,timeout)); // remove crypto ?
	
	rc_close();
	
}

