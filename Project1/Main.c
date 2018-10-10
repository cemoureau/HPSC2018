#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

#define DEBUG 0

/*=======================================================================================
*	This code was written by: Antonin Aumètre - antonin.aumetre@gmail.com
*	For: High Performance Scientific course at ULiège, 2018-19
*	Project 1
*
*	Under GNU General Public License 09/2018
=======================================================================================*/


	//======================= PRE-PROCESSING ===========================//
	//Reads the content of the image file and stores it in an array

	int compare (const void * a, const void * b) {
   		return ( *(int*)a - *(int*)b );
	}


	int main(int argc, char **argv){
	
	printf("Usage: ./Main <file_name.ppm> <(int)Filter_size> <(int)Filter_level>\n-----------------\n");
	//Get some parameters
	char *filename = argv[1];
	int Fs = atoi(argv[2]);
	int Fl = atoi(argv[3]);

	printf("File: %s\n", filename);
	printf("Fs: %d\n", Fs);
	printf("Fl: %d\n", Fl);

	int depth = 0, width = 0, height = 0;
	long fileLength = 0;

	//Read the file
	FILE *pFile; //File pointer
	pFile = fopen(filename, "rb"); //Opens the file  
	if (pFile == NULL) {//Checks if the file was opened correctly
		printf("Error while opening the file.\n");
		exit(0);
	}

	//Read the next parameters as one string
	char c, str[40];
	int j=0; // CHANGE THIS VARIABLE'S NAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	for (int i=0; i< 4; ++i){
			c = fgetc(pFile);
		while ((c != '\n') && (c != '\r')&&(c != '\0') && (c != ' ')){
			str[j] = c;
			c = fgetc(pFile);
			++j;
		} 
		str[j] = ' ';
		++j;
	}
	str[j+1] = '\0';

	//Separates the strings and assign values
	char s_width[8], s_height[8], s_depth[4], s_magic[3];
	j = 0;
	int rep = 0;

	//Make a fucking loop !!
	while ((str[j] != ' ') && (str[j] != '\r') && (str[j] != '\0') && (str[j] != '\n')){
		s_magic[j-rep] = str[j];
		++j;
	}
	s_magic[j-rep] = '\0';
	if ((s_magic[0] != 'P') || (s_magic[1] != '6')){
		printf("Wrong file format. Aborting ...\n");
		exit(0);
	}
	++j;
	rep = j;

	while ((str[j] != ' ') && (str[j] != '\r') && (str[j] != '\0') && (str[j] != '\n')) {
		s_width[j-rep] = str[j];
		++j;
	}
	s_width[j] = '\0';
	width = atoi(s_width);
	++j;
	rep = j;
	while ((str[j] != ' ') && (str[j] != '\r') && (str[j] != '\0') && (str[j] != '\n')){
		s_height[j-rep] = str[j];
		++j;
	}
	s_height[j-rep] = '\0';
	height = atoi(s_height);
	++j;
	rep = j;

	while ((str[j] != ' ') && (str[j] != '\r') && (str[j] != '\0') && (str[j] != '\n')){
		s_depth[j-rep] = str[j];
		++j;
	}
	s_depth[j-rep] = '\0';
	depth = atoi(s_depth);

	printf("Image properties\n-----------------\nWidth : %d\nHeight: %d\nColor depth: %d\n-----------------\n", width, height, depth);
	
	//Getting the file length
	fseek(pFile, 0, SEEK_END);
	fileLength = ftell(pFile);
	if(DEBUG)printf("File length: %ld\n", fileLength);
	fseek(pFile, 0, SEEK_SET);

	//Allocate memory
	unsigned char * buffer = (char *)malloc(fileLength+1); //Creates a buffer
	if(DEBUG)printf("Buffer created\n");
	//Read
	fread(buffer, fileLength+1, 1, pFile);
	fclose(pFile);

	
	int offset = j+1; //Number of bytes preceeding the image data
	unsigned char * pic = (unsigned char *)malloc(3*height*width*sizeof(char));


	//Create an array of integers instead of binary values
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			for (int k = 0; k < 3 ; ++k) {
				pic[3*(i*width + j) + k] = buffer[offset + 3*(i*width + j) + k];
			}
		}
	}

	free(buffer);


	//======================= ALGORITHM ===========================//
	/*
	Input :
		- filter size Fs
		- filter level Fl
		- width and height extracted from the file
		- array pic[width][height]
	Output :
		- array containing the filtered image
	*/
	
	printf("Algorithm started...\n");
	unsigned char * newPic = (unsigned char *)malloc(3*height*width*sizeof(char));

	int k=0;
	//Creates a square mask
	int adresses[4*Fs*Fs][2]; //Declare a larger than needed array
	for (int a = Fs; a >= -Fs; --a) {
		for (int b = Fs; b >= -Fs; --b) {
			if (a*a + b*b <= Fs*Fs) {
				//Append (a,b) to adresses
				adresses[k][0] = a;
				adresses[k][1] = b;
				++k;
			}
		}
	} //We now have k neighbors
	int numthreads = 1;
	clock_t end = clock();
	double time_spent;

	for (int num_threads=1 ; num_threads<=16 ; ++num_threads){
		omp_set_num_threads(num_threads);
		clock_t begin = clock();

	#pragma omp parallel 
		{
			int actual_neighbors = 0;
			int temp_Pic[k][3];//Create a local copy of the useful portion of the image
			int averageC[depth+1][3];
			int intensityCount[depth+1];
			int RGB_max[3]={0,0,0};
			int curMax = 0;

			numthreads = __builtin_omp_get_num_threads();
			#pragma omp for
			//Applying the algorithm to the whole image
			for (int i = 0; i < height; ++i){
				for (int j = 0; j < width; ++j) {

					actual_neighbors = 0;
					for (int l = 0; l < k; ++l) {
						//Check if the neighbor is not outside the picture
						if ((i + adresses[l][1] >= 0) && (i + adresses[l][1] < height) && (j + adresses[l][0] >= 0) && (j + adresses[l][0] < width)) {
							temp_Pic[actual_neighbors][0] = pic[3*((i + adresses[l][1])*width + j + adresses[l][0]) + 0];
							temp_Pic[actual_neighbors][1] = pic[3*((i + adresses[l][1])*width + j + adresses[l][0]) + 1];
							temp_Pic[actual_neighbors][2] = pic[3*((i + adresses[l][1])*width + j + adresses[l][0]) + 2];
							if (DEBUG)printf("%d %d %d\n", temp_Pic[actual_neighbors][0], temp_Pic[actual_neighbors][1], temp_Pic[actual_neighbors][2]);
							actual_neighbors++;
						}
					}

					if (DEBUG)printf("Actual neighbors found: %d\n", actual_neighbors);

					for (int l=0 ; l<depth+1; ++l){
						averageC[l][0] = 0;
						averageC[l][1] = 0;
						averageC[l][2] = 0;
						intensityCount[l]=0;
					}
					RGB_max[0]=0;
					RGB_max[1]=0;
					RGB_max[2]=0;
					curMax = 0;
					for (int l=0 ; l<actual_neighbors; ++l){
						int curIntensity = floor((temp_Pic[l][0] + temp_Pic[l][1] + temp_Pic[l][2]) / (3 * Fl));
						if (DEBUG)printf("Current intensity: %d\n", curIntensity);
						intensityCount[curIntensity]++;
						if (intensityCount[curIntensity]>curMax)curMax=intensityCount[curIntensity];
						averageC[curIntensity][0] += temp_Pic[l][0];
						averageC[curIntensity][1] += temp_Pic[l][1];
						averageC[curIntensity][2] += temp_Pic[l][2];
						//These lines are responsible for the change in appearance
						if (averageC[curIntensity][0] > RGB_max[0])RGB_max[0]=averageC[curIntensity][0];//Red
						if (averageC[curIntensity][1] > RGB_max[1])RGB_max[1]=averageC[curIntensity][1];//Green
						if (averageC[curIntensity][2] > RGB_max[2])RGB_max[2]=averageC[curIntensity][2];//Blue
					}

					if (DEBUG)printf("Max values: %d %d %d\n", RGB_max[0], RGB_max[1], RGB_max[2]);

					newPic[3*(i*width + j) + 0] = RGB_max[0] / curMax;
					newPic[3*(i*width + j) + 1] = RGB_max[1] / curMax;
					newPic[3*(i*width + j) + 2] = RGB_max[2] / curMax;

					if (DEBUG)printf("New pixels: %d %d %d\n", newPic[3*(i*width + j) + 0], newPic[3*(i*width + j) + 1], newPic[3*(i*width + j) + 2]);
				}
			}
		}
		
		end = clock();
		time_spent = (double)(end - begin) / (CLOCKS_PER_SEC * numthreads);
		printf("\nJob done in %2.4lf s, using %d threads.\n", time_spent, numthreads);
	}
	free(pic);


	//======================= POST-PROCESSING ===========================//
	//Takes the filtred image and saves it as a .ppm
	
	char oily_filename[9] = "oily.ppm";
	//strcat(oily_filename, filename);

	FILE *pNewFile = fopen(oily_filename, "wb");
	//Writing the header
	char header[offset], temp[6];
	for (int i =0; i<offset ; ++i)header[i]='\0';
	sprintf(temp, "P6\n");
	strcat(header, temp);
	sprintf(temp, "%d", width);
	strcat(header, temp);
	strcat(header, " ");
	sprintf(temp, "%d", height);
	strcat(header, temp);
	strcat(header, "\n");
	sprintf(temp, "%d", depth);
	strcat(header, temp);
	strcat(header, "\n");
	strcat(header, "\0");
	fwrite(header, 1, offset, pNewFile);

	//Writing the data contained in pic
	unsigned char newBuffer[3*width]; //Creates another buffer
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			newBuffer[3*j]   =   newPic[3*(i*width + j)];//Red
			newBuffer[3*j + 1] = newPic[3*(i*width + j)+1];//Green
			newBuffer[3*j + 2] = newPic[3*(i*width + j)+2];// Blue
		}
		fwrite(newBuffer, 1, sizeof(newBuffer), pNewFile);
	}
	fclose(pNewFile);
	free(newPic);
	//======================= END OF PROGRAM ===========================//

	return(0);
}