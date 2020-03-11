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
#define WAIT_THREAD_FINISH { for (int i  = 0; i < THREAD_NUM; i++) pthread_join(thread[i], NULL); }
sem_t mutex;
struct parameter {
	int start;
};

int imgWidth, imgHeight;
int FILTER_SIZE;
int FILTER_SCALE;
int *filter_G;

const char *inputfile_name[5] = {
	"input1.bmp",
	"input2.bmp",
	"input3.bmp",
	"input4.bmp",
	"input5.bmp"
};
const char *outputBlur_name[5] = {
	"Blur1.bmp",
	"Blur2.bmp",
	"Blur3.bmp",
	"Blur4.bmp",
	"Blur5.bmp"
};

unsigned char *pic_in, *pic_grey, *pic_blur, *pic_final;

unsigned char RGB2grey(int w, int h)
{
	int target_pos = 3*(h*imgWidth + w);
	int tmp = (
		pic_in[target_pos + MYRED] +
		pic_in[target_pos + MYGREEN] +
		pic_in[target_pos + MYBLUE] )/3;

	if (tmp > 255) tmp = 255;
	return (unsigned char)tmp;
}

unsigned char GaussianFilterEdge(int w, int h)
{
	register int  tmp = 0;
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
	
			tmp += filter_G[x++] * pic_grey[b + a++];
		}
	}
	tmp /= FILTER_SCALE;
	if (tmp > 255) tmp = 255;
	
	x = 3*(h*imgWidth + w);
	pic_final[x+MYRED] = tmp;
	pic_final[x+MYGREEN] = tmp;
	pic_final[x+MYBLUE] = tmp;

	return (unsigned char)tmp;
}

unsigned char GaussianFilter(int w, int h)
{
	register int tmp = 0;
	register int a, b;
	int ws = (int)sqrt((float)FILTER_SIZE);
	int ws_2 = ws/2;
	register int x = 0;

	for (register int j = 0; j<ws; ++j) {
		b = (h+j-ws_2) * imgWidth;
		a = b + w - ws_2;
		for (register int i = 0; i<ws; ++i)
			tmp += filter_G[x++] * pic_grey[a++];
	}
	tmp /= FILTER_SCALE;
	if (tmp > 255) tmp = 255;

	x = 3*(h*imgWidth + w);
	pic_final[x+MYRED] = tmp;
	pic_final[x+MYGREEN] = tmp;
	pic_final[x+MYBLUE] = tmp;
	
	return (unsigned char)tmp;
}


//*******************************************************************************
//*                            Function for Threads                             *
//*******************************************************************************

void* threadGrey(void* ptr) {
	struct parameter *para = (struct parameter*) ptr;
	int start = para->start;
	register int x;

	for (register int j = start; j < imgHeight; j+=THREAD_NUM) {
		x = j*imgWidth;
		for (register int i = 0; i < imgWidth; ++i){
			pic_grey[x++] = RGB2grey(i, j);
		}
	}
	sem_post(&mutex);
}
void* threadFilterColEdge(void* ptr) {
	struct parameter *para = (struct parameter*) ptr;
	int start = para->start;
	int ws = (int)sqrt((float)FILTER_SIZE);
	int ws_2 = ws/2;

	for (register int j = start+ws_2; j < imgHeight-ws_2; j+=THREAD_NUM) {
		for (register int i = 0; i < ws_2; ++i) {
			GaussianFilterEdge(i, j);
		}
		for (register int i = imgWidth-ws_2; i < imgWidth; ++i) {
			GaussianFilterEdge(i, j);
		}
	}
}
void* threadFilter(void* ptr) {
	struct parameter *para = (struct parameter*) ptr;
	int start = para->start;
	int ws = (int)sqrt((float)FILTER_SIZE);
	register int ws_2 = ws/2;
	
	for (register int j = start+ws_2; j < imgHeight-ws_2; j+=THREAD_NUM) {
		for (register int i = ws_2; i < imgWidth-ws_2; ++i){
			GaussianFilter(i, j);
		}
	}
}

//*******************************************************************************


int main()
{
	// read mask file
	FILE* mask;
	mask = fopen("mask_Gaussian.txt", "r");
	fscanf(mask, "%d", &FILTER_SIZE);
	fscanf(mask, "%d", &FILTER_SCALE);

	filter_G = new int[FILTER_SIZE];
	for (int i = 0; i<FILTER_SIZE; i++)
		fscanf(mask, "%d", &filter_G[i]);
	fclose(mask);

	BmpReader* bmpReader = new BmpReader();
	for (int k = 0; k<5; k++){
		// read input BMP file
		pic_in = bmpReader->ReadBMP(inputfile_name[k], &imgWidth, &imgHeight);
		// allocate space for output image
		pic_grey = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_final = (unsigned char*)malloc(3 * imgWidth*imgHeight*sizeof(unsigned char));

		// initialize semephore
		sem_init(&mutex, 0, 0);

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
			pthread_create(thread+i, NULL, threadGrey, (void*) (para+i));
		}
		for (int i = 0; i < THREAD_NUM; i++) {
			sem_wait(&mutex);
		}
		// WAIT_THREAD_FINISH;

		//*******************************************************************************
		//*                           Apply Guassian filter                             *
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
				GaussianFilterEdge(i, j);
			}
		}
		for (int j = imgHeight-ws_2; j < imgHeight; ++j) {
			for (int i = 0; i < imgWidth; ++i) {
				GaussianFilterEdge(i, j);
			}
		}

		WAIT_THREAD_FINISH;

		// write output BMP file
		bmpReader->WriteBMP(outputBlur_name[k], imgWidth, imgHeight, pic_final);

		//free memory space
		free(pic_in);
		free(pic_grey);
		free(pic_final);
	}

	sem_destroy(&mutex);

	return 0;
}

