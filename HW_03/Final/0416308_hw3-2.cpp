// Student ID:
// Name      :
// Date      : 2017.11.03

#include "bmpReader.h"
#include "bmpReader.cpp"
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
using namespace std;

#define MYRED	2
#define MYGREEN 1
#define MYBLUE	0


#define THREAD_NUM 12
#define WAIT_THREAD_FINISH { for (int i = 0; i < THREAD_NUM; i++) pthread_join(thread[i], NULL); }
pthread_mutex_t mutex[THREAD_NUM];
struct parameter {
	int start;
	// int end;
};


int imgWidth, imgHeight;
int FILTER_SIZE;
int *filter_Gx, *filter_Gy;

const char *inputfile_name[5] = {
	"input1.bmp",
	"input2.bmp",
	"input3.bmp",
	"input4.bmp",
	"input5.bmp"
};

const char *outputSobel_name[5] = {
	"Sobel1.bmp",
	"Sobel2.bmp",
	"Sobel3.bmp",
	"Sobel4.bmp",
	"Sobel5.bmp"
};


unsigned char *pic_in, *pic_grey, *pic_sobel, *pic_final;

unsigned char RGB2grey(int w, int h)
{
	int target_pos = 3 * (h*imgWidth + w);
	int tmp = (
		pic_in[target_pos + MYRED] +
		pic_in[target_pos + MYGREEN] +
		pic_in[target_pos + MYBLUE] )/3;

	if (tmp > 255) tmp = 255;
	return (unsigned char)tmp;
}

unsigned char SobelFilterEdge(int w, int h)
{
	register int img_x = 0, img_y = 0;
	register unsigned char result;
	register float get;
	register int a, b;
	int ws = (int)sqrt((float)FILTER_SIZE);
	int ws_2 = ws/2;
	register int x = 0;

	for (register int j = 0; j<ws; j++) {
		b = h + j - ws_2;
		if (b < 0 || b >= imgHeight) {
			x += ws;
			continue;
		}
		b *= imgWidth;
		a = w - ws_2;
		for (register int i = 0; i<ws; i++)
		{
			// detect for borders of the image
			if (a<0 || a>=imgWidth) {
				++a, ++x;
				continue;
			}

			img_x += filter_Gx[x] * pic_grey[b + a];
			img_y += filter_Gy[x++] * pic_grey[b + a++];
		}
	}
	if (img_x < 0) img_x = 0;
	if (img_y < 0) img_y = 0;
	get = sqrt(img_x*img_x + img_y*img_y);
	if (get > 255) result = 255;
	else result = (unsigned char) get;
	
	x = 3 * (h*imgWidth + w);
	pic_final[x+MYRED] = result;
	pic_final[x+MYGREEN] = result;
	pic_final[x+MYBLUE] = result;

	return result;
}

unsigned char SobelFilter(int w, int h) {
	register int img_x = 0, img_y = 0;
	register unsigned char result;
	register float get;
	register int a, b;
	int ws = (int)sqrt((float)FILTER_SIZE);
	int ws_2 = ws/2;
	register int x = 0;

	for (register int j = 0; j < ws; j++) {
		b = (h+j-ws_2) * imgWidth;
		a = b + w - ws_2;
		for (register int i = 0; i < ws; i++)
		{
			img_x += filter_Gx[x] * pic_grey[a];
			img_y += filter_Gy[x++] * pic_grey[a++];
		}
	}
	if (img_x < 0) img_x = 0;
	if (img_y < 0) img_y = 0;
	get = sqrt(img_x*img_x + img_y*img_y);
	if (get > 255) result = 255;
	else result = (unsigned char) get;

	x = 3 * (h*imgWidth + w);
	pic_final[x+MYRED] = result;
	pic_final[x+MYGREEN] = result;
	pic_final[x+MYBLUE] = result; 

	return result;
}


//**************************************************************************
//*                           Thread Function                              *
//**************************************************************************
void* threadGrey(void* ptr) {
	struct parameter *para = (struct parameter*) ptr;
	int start = para->start;
	register int x;
	
	for (register int j = start; j < imgHeight; j+=THREAD_NUM) {
		x = j * imgWidth;
		for (register int i = 0; i < imgWidth; ++i) {
			pic_grey[x++] = RGB2grey(i, j);
		}
	}
	pthread_mutex_unlock(mutex+start);
}		
void* threadFilterColEdge(void* ptr) {
	struct parameter *para = (struct parameter*) ptr;
	int start = para->start;
	int ws = (int)sqrt((float)FILTER_SIZE);
	int ws_2 = ws/2;

	for (register int j = start+ws_2; j < imgHeight-ws_2; j+=THREAD_NUM) {
		for (register int i = 0; i < ws_2; ++i)
			SobelFilterEdge(i, j);
		for (register int i = imgWidth-ws_2; i < imgWidth; ++i)
			SobelFilterEdge(i, j);
	}
}
void* threadFilter(void* ptr) {
	struct parameter *para = (struct parameter*) ptr;
	int start = para->start;
	int ws = (int)sqrt((float)FILTER_SIZE);
	int ws_2 = ws/2;
	int endh = imgHeight - ws_2;
	int endw = imgWidth - ws_2;

	for (register int j = start+ws_2; j < endh; j+=THREAD_NUM) {
		for (register int i = ws_2; i < endw; ++i){
			SobelFilter(i, j);
		}
	}
}

//**************************************************************************


int main()
{
	// read mask file
	FILE* mask;
	mask = fopen("mask_Sobel.txt", "r");
	fscanf(mask, "%d", &FILTER_SIZE);

	filter_Gx = new int[FILTER_SIZE];
	filter_Gy = new int[FILTER_SIZE];
	for (int i = 0; i<FILTER_SIZE; i++)
		fscanf(mask, "%d", &filter_Gx[i]);
	for (int i = 0; i<FILTER_SIZE; i++)
		fscanf(mask, "%d", &filter_Gy[i]);
	fclose(mask);

	// initialize mutex lock
	for (int i = 0; i < THREAD_NUM; i++) {
		mutex[i] = PTHREAD_MUTEX_INITIALIZER;
	}

	BmpReader* bmpReader = new BmpReader();
	for (int k = 0; k<5; k++){
		// read input BMP file
		pic_in = bmpReader->ReadBMP(inputfile_name[k], &imgWidth, &imgHeight);
		// allocate space for output image
		pic_grey = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		// pic_sobel = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_final = (unsigned char*)malloc(3 * imgWidth*imgHeight*sizeof(unsigned char));
		
		//*******************************************************************************
		//*                          Set parameters for pthread                         *
		//*******************************************************************************
		pthread_t thread[THREAD_NUM];
		struct parameter para[THREAD_NUM];

		for (int i = 0; i < THREAD_NUM; i++)
			para[i].start = i;

		//*******************************************************************************
		//*                         Convert to grey scale photo                         *
		//*******************************************************************************
		for (int i = 0; i < THREAD_NUM; i++) {
			pthread_mutex_lock(mutex+i);
			pthread_create(thread+i, NULL, threadGrey, (void*) (para+i));
		}
		for (int i = 0; i < THREAD_NUM; i++) {
			pthread_mutex_lock(mutex+i);
			pthread_mutex_unlock(mutex+i);
		}
		// WAIT_THREAD_FINISH;

		//*******************************************************************************
		//*                            Apply Sobel filter                               *
		//*******************************************************************************
		int ws = (int)sqrt((float)FILTER_SIZE);
		int ws_2 = ws/2;

		// deal with edge on right and left handside
		for (int i = 0; i < THREAD_NUM; i++) {
			pthread_create(thread+i, NULL, threadFilterColEdge, (void*) (para+i));
		}
		WAIT_THREAD_FINISH;
		
		for (int i = 0; i < THREAD_NUM; i++) {
			pthread_create(thread+i, NULL, threadFilter, (void*) (para+i));
		}

		// deal with edge on top and buttom
		for (int j = 0; j < ws_2; ++j) {
			for (int i = 0; i < imgWidth; ++i) {
				SobelFilterEdge(i, j);
			}
		}
		for (int j = imgHeight-ws_2; j < imgHeight; ++j) {
			for (int i = 0; i < imgWidth; ++i) {
				SobelFilterEdge(i, j);
			}
		}

		WAIT_THREAD_FINISH;

		// write output BMP file
		bmpReader->WriteBMP(outputSobel_name[k], imgWidth, imgHeight, pic_final);

		//free memory space
		free(pic_in);
		free(pic_grey);
		// free(pic_sobel);
		free(pic_final);
	}

	delete bmpReader;
	delete filter_Gx;
	delete filter_Gy;
	for (int i = 0; i < THREAD_NUM; i++) {
		pthread_mutex_destroy(mutex+i);
	}

	return 0;
}
