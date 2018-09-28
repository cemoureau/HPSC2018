#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define DEBUG 0

/*=======================================================================================
*	This code was written by: Antonin Aumètre
*	For: High Performance Scientific course at ULiège, 2018-19
*
*	Under
=======================================================================================*/


int main(int argc, char **argv){
	//======================= PRE-PROCESSING ===========================//
	//Reads the content of the image file and stores it in an array
	/*TODO :
		- do everything in a cleaner way
		- generalize offset size
	*/
	
	printf("Usage: ./Main <file_name> <Fs> <Fl>\n-----------------\n");
	//Get some parameters
	char *filename = argv[1];
	int Fs = atoi(argv[2]);
	int Fl = atoi(argv[3]);

	printf("File: %s\n", filename);
	printf("Fs: %d\n", Fs);
	printf("Fl: %d\n", Fl);

	int depth = 0, width = 0, height = 0;
	long fileLength = 0;
	char param1[8], param2[4], param3[4];//Needs to be generalized

	//Read the file
	FILE *pFile; //File pointer
	strcat(filename, ".ppm");
	pFile = fopen(filename, "rb"); //Opens the file  
	if (pFile == NULL) {//Checks if the file was opened correctly
		printf("Error while opening the file.\n");
		exit(0);
	}

	//Extract the image parameters from the .ppm file
	fgets(param1, 4, pFile);//P6
	if (param1[0] != 'P' || param1[1] != '6') {
		printf("Wrong file format.\n");
		exit(0);
	}
	fgets(param1, 8, pFile);//Width	Height
	for (int i = 0; i < 3; ++i) {//Extract height
		param2[i] = param1[i];//To be generalized for any size
	}
	param2[3] = '\0';
	for (int i = 0; i < 3; ++i) {//Extract height
		param3[i] = param1[4+i];//To be generalized for any size
	}
	param3[3] = '\0';
	width = atoi(param2);
	height = atoi(param3);
	fgets(param1, 4, pFile);//Carrier return
	fgets(param1, 4, pFile);//Depth
	depth = atoi(param1);
	printf("Image properties\n-----------------\nWidth : %d\nHeight: %d\nColor depth: %d\n-----------------\n", width, height, depth);
	//Getting the file length
	fseek(pFile, 0, SEEK_END);
	fileLength = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	//Allocate memory
	unsigned char buffer[fileLength+1]; //Creates a buffer
	//Read
	fread(buffer, fileLength+1, 1, pFile);
	fclose(pFile);

	int offset = 15; //Number of bytes preceeding the image data
	unsigned char pic[height][width][3]; //(x, y, c)
	//Create an array of integers instead of binary values
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			for (int k = 0; k < 3 ; ++k) {
				pic[i][j][k] = buffer[offset + 3*i*width + 3*j + k];
			}
		}
	}
	clock_t begin = clock();

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
	unsigned char newPic[height][width][3];

	//Creates a square mask
	int adresses[2*4*Fs*Fs]; //Declare a larger than needed array
	int k = 0;
	for (int a = Fs; a >= -Fs; --a) {
		for (int b = Fs; b >= -Fs; --b) {
			//printf("a = %d :: b = %d\n", a, b);
			if (a*a + b*b <= Fs*Fs) {
				//Append (a,b) to adresses
				adresses[k] = a;
				adresses[k + 1] = b;
				k += 2;
			}
		}
	} //We now have k elements in "adresses" and k/2 neighbors

	//Applying the algorithm to the whole image
	for (int i = 0; i < height; ++i) {
		float progress = (float)100.0*(i+1.0)/height;
		printf("%.1f %%\n", progress);
		for (int j = 0; j < width; ++j) {
			//Get the neighboring pixels for (i,j)
			//REMARK : the neighbors include (i,j)
			int actual_neighbors = 0;
			int neighbors[k]; //Contains PIXEL adresses
			if (DEBUG)printf("Line: %d, row: %d\n", i, j);

			for (int l = 0; l < k; l += 2) {
				//Check if the neighbor is not outside the picture
				if ((i + adresses[l+1] >= 0) && (i + adresses[l+1] < height) && (j + adresses[l] >= 0) && (j + adresses[l] < width)) {
					neighbors[2*actual_neighbors] = j + adresses[l];// x value
					neighbors[2*actual_neighbors + 1] = i + adresses[l+1];// y value
					actual_neighbors++;
				}
			}

			if (DEBUG){
				printf("Actual neighbors: %d\n", actual_neighbors);
				for (int l = 0; l < 2*actual_neighbors; l+=2) {
					printf("Position(%d:%d) ", neighbors[l+1], neighbors[l]);
					printf("Color: %d %d %d\n", pic[neighbors[l]][neighbors[l + 1]][0], pic[neighbors[l]][neighbors[l + 1]][1], pic[neighbors[l]][neighbors[l + 1]][2]);
				}
			}
			
			//Compute the color intensity (int)
			int intensities[actual_neighbors];
			int R = 0, G = 0, B = 0;
			for (int l = 0; l < 2*actual_neighbors; l+=2) {
				R = pic[neighbors[l]][neighbors[l + 1]][0];
				G = pic[neighbors[l]][neighbors[l + 1]][1];
				B = pic[neighbors[l]][neighbors[l + 1]][2];
				intensities[l / 2] = floor((R + G + B) / (3 * Fl));
			}
			
			if (DEBUG){
				printf("Intensities: ");
				for (int l = 0; l < actual_neighbors; l++) printf("%d ", intensities[l]);
					printf("\n");
			}

			//Count occurences of Ik in the set of intensities
			int occurences[depth+1];
			for (int l = 0; l <= depth; ++l) {
				occurences[l] = 0;
			}
			for (int l = 0; l <= floor(depth/Fl)+1; ++l) {
				for (int m = 0; m < actual_neighbors; ++m) {
					if (intensities[m] == l) {
						occurences[l]++;
					}
				}
			}

			//Compute the color intensities
			int Irgb[3][depth+1];

			for (int m = 0; m <= depth; ++m) {
				for (int l = 0; l < 3; ++l) {
					Irgb[l][m] = 0;
				}
			}

			// ça pue ici !!!!!!!!
			for (int m = 0; m <= depth; ++m) {//For each intensity level (256 values)
				for (int l = 0; l < 2*actual_neighbors; l+=2) {
					if (intensities[l/2] == m){
						Irgb[0][m] += pic[neighbors[l]][neighbors[l+1]][0];// Red
						Irgb[1][m] += pic[neighbors[l]][neighbors[l+1]][1];// Green
						Irgb[2][m] += pic[neighbors[l]][neighbors[l+1]][2];// Blue
						if (DEBUG)printf("Intensity: %d, Sum:%d\n", intensities[l], Irgb[0][m]);
					}
				}
			}

			//Find max(occurences)
			int Imax = 0;
			for (int l = 0; l <= depth; ++l) {
				if (occurences[l] > Imax) {
					Imax = occurences[l];
				}
			}
			if (DEBUG)printf("Max intensity: %d\n", Imax);

			//Find max(colors intensity)
			int I_max_rgb[3]={0,0,0};
			for (int l = 0; l <= depth; ++l) {
				if (Irgb[0][l] > I_max_rgb[0])I_max_rgb[0]=Irgb[0][l];//Red
				if (Irgb[1][l] > I_max_rgb[1])I_max_rgb[1]=Irgb[1][l];//Green
				if (Irgb[2][l] > I_max_rgb[2])I_max_rgb[2]=Irgb[2][l];//Blue
			}

			if(DEBUG)printf("Rm:%d Gm:%d Bm:%d\n", I_max_rgb[0], I_max_rgb[1], I_max_rgb[2]);
			//Assign new values
			newPic[i][j][0] = floor(I_max_rgb[0] / Imax);//Red
			newPic[i][j][1] = floor(I_max_rgb[1] / Imax);//Green
			newPic[i][j][2] = floor(I_max_rgb[2] / Imax);//Blue
			if (DEBUG){
				printf("New pixel values: %d %d %d\n", newPic[i][j][0], newPic[i][j][1], newPic[i][j][2]);
				printf("================================\n");
			}
		}
	}
	clock_t end = clock();
	double time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
	printf("\nJob done in %2.4lf s !\n", time_spent);

	//======================= POST-PROCESSING ===========================//
	//Takes the filtred image and saves it as a .ppm
	
	char oily_filename[20] = "oily_";
	strcat(oily_filename, filename);

	FILE *pNewFile = fopen(oily_filename, "wb");
	//Writing the header
	char header[15] = "P6\n", temp[4];
	sprintf(temp, "%d", height);
	strcat(header, temp);
	strcat(header, " ");
	sprintf(temp, "%d", width);
	strcat(header, temp);
	strcat(header, "\n");
	sprintf(temp, "%d", depth);
	strcat(header, temp);
	strcat(header, "\n");
	fwrite(header, 1, sizeof(header), pNewFile);
	//Writing the data contained in pic
	unsigned char newBuffer[3*height*width]; //Creates another buffer

	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			newBuffer[3*(width*i + j)]   =   newPic[j][i][0];//Red
			newBuffer[3*(width*i + j) + 1] = newPic[j][i][1];//Green
			newBuffer[3*(width*i + j) + 2] = newPic[j][i][2];// Blue
		}
	}
	fwrite(newBuffer, 1, sizeof(newBuffer), pNewFile);
	fclose(pNewFile);

	//======================= END OF PROGRAM ===========================//
	return(0);
}