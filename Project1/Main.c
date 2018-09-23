#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#pragma warning(disable : 4996) //Used for VisualStudio, to avoid fopen_s Error

int main(){
	//======================= PRE-PROCESSING ===========================//
	//Reads the content of the image file and stores it in an array
	/*TODO :
		- change Fs and Fl to be callable
		- change fopen argument to be callable
		- generalize size of width/height parameters
		- store the data in pic
		- do everything in a cleaner way
	*/
	char filename[] = "lena";

	int Fs = 4, Fl = 2, depth = 0, width = 0, height = 0;
	long fileLength = 0;
	char param1[8], param2[4], param3[4];

	//system("color 0a");
	FILE *pFile; //File pointer
	pFile = fopen("lena.ppm", "rb"); //Opens the file  
	if (pFile == NULL) {//Checks if the file was opened correctly
		printf("Error while opening the file.\n");
		exit(EXIT_FAILURE);
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
	printf("Image properties\n-----------------\nWidth : %d\nHeight: %d\nDepth: %d\n-----------------\n", width, height, depth);
	
	//Getting the file length
	fseek(pFile, 0, SEEK_END);
	fileLength = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	//Allocate memory
	unsigned char * buffer = (char *)malloc(fileLength+1); //Creates a buffer
	//Read
	fread(buffer, fileLength+1, 1, pFile);
	fclose(pFile);


	int offset = 16; //Number of bytes preceeding the image data
	unsigned char **pic = (unsigned char **)malloc(height*sizeof(char *));
	for (int i = 0; i < height; ++i)
		pic[i] = (unsigned char *)malloc(3*width * sizeof(unsigned char));
	//Create an array of integers instead of binary values
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			pic[i][j] =   buffer[offset + i*3*width + 3*j];//Red value
			pic[i][j+1] = buffer[offset + i*3*width + 3*j + 1];//Green value
			pic[i][j+2] = buffer[offset + i*3*width + 3*j + 2];//Blue value
		}
	}
	
	printf("Buffer: ");
	for (int j = 0; j < 20; j+=3) {
		printf("%d %d %d |", buffer[offset+j], buffer[offset+j+1], buffer[offset+j+2]);
	}
	printf("\n");

	printf("Array:  ");
	for (int j = 0; j < 20; j+=3) {
		printf("%d %d %d |", pic[0][j], pic[0][j+1], pic[0][j+2]);
	}
	printf("\n");
	

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

	//Compute the neighbor's positions
	/*
	int *adresses = (int *)malloc(2 * Fs*Fs * sizeof(int)); //Declare a larger than needed array
	int k = 0;
	for (int a = -Fs; a < Fs; ++a) {
		for (int b = -Fs; b < Fs; ++b) {
			if (a*a + b * b <= Fs) {
				//Append (a,b) to adresses
				adresses[k] = a;
				adresses[k + 1] = b;
				k += 2;
			}
		}
	} //We now have k elements in "adresses"
	//Resizing the "adresses" array:
	adresses = (int *)realloc(adresses, k * sizeof(int));

	int *neighbors = (int *)malloc(k * sizeof(int));
	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
			//Get the neighboring pixels for (i,j)
			//REMARK : the neighbors include (i,j)
			for (int l = 0; l < k; l += 2) {
				//Check if the neighor is not outside the picture
				if ((i + adresses[l] >= 0) && (i + adresses[l] < width)) {
					neighbors[l] = i + adresses[l];
				}
				if ((j + adresses[l + 1] >= 0) && (j + adresses[l + 1] < height)) {
					neighbors[l + 1] = j + adresses[l + 1];
				}
			}
			//Compute the color intensity (int)
			int *intensities = (int *)malloc(k / 2 * sizeof(int));
			int R = 0, G = 0, B = 0;
			for (int l = 0; l < k; l += 2) {
				R = pic[neighbors[l]][3 * neighbors[l + 1]];
				G = pic[neighbors[l]][3 * neighbors[l + 1] + 1];
				B = pic[neighbors[l]][3 * neighbors[l + 1] + 2];
				intensities[l / 2] = round((R + G + B) / (3 * Fl));
			}
			//Count occurences of Ik in the set of intensities
			int occurences[depth];
			for (int l = 0; l <= depth; ++l) {
				for (int m = 0; m < k / 2; ++m) {
					if (intensities[m] == l) {
						occurences[l]++;
					}
				}
			}
			//Compute the color intensities
			int *Irgb[3];
			for (i = 0; i < 3; i++)
				Irgb[i] = (int *)malloc(depth * sizeof(int));
			for (int l = 0; l <= depth; ++l) {
				for (int m = 0; m < occurences[l]; ++m) {
					//Sum over each color	
					Irgb[0][l] += r(a, b);
					Irgb[1][l] += g(a, b);
					Irgb[2][l] += b(a, b);

				}
			}
			//Find max(occurences)
			int Imax = 0;
			for (int l = 0; l <= depth; ++l) {
				if (occurences[l] > Imax) {
					Imax = occurences[l];
				}
			}
			//Assigning pixel values
			int *newPic[3];
			for (i = 0; i < 3; i++)
				Irgb[i] = (int *)malloc(depth * sizeof(int));
		}
	}
				*/
	//======================= POST-PROCESSING ===========================//
	//Takes the filtred image and saves it as a .ppm
	/*TODO:
		- get colors (lol)	
	*/
	char oily_filename[20] = "oily_";
	strcat(oily_filename, filename);
	strcat(oily_filename, ".ppm");

	FILE *pNewFile = fopen(oily_filename, "wb");
	//Writing the header
	char header[16] = "P6\n", temp[4];
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
	char * newBuffer = (char *)malloc(3*8*height*width); //Creates another buffer

	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			newBuffer[width*i*3 + 3*j] = pic[i][j];//Green
			newBuffer[width*i*3 + 3*j + 1] = pic[i][j+1];//Blue
			newBuffer[width*i*3 + 3*j + 2] = pic[i][j+2];// Red 
		}
	}
	fwrite(newBuffer, 1, fileLength-offset, pNewFile);
	fclose(pNewFile);

	//======================= END OF PROGRAM ===========================//
	free(buffer);
	free(newBuffer);
	//free(adresses);
	free(pic);	 
	//system("PAUSE"); //For Windows only
	return(0);
}