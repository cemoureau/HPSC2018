#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#pragma warning(disable : 4996) //Used for VisualStudio, to avoid fopen_s Error

/*
char *pixels;
pixels = (char *)malloc(dataSize*sizeof(char));


typedef struct Image{
	unsigned int width;
	unsigned int height;
	char *pixels;
}Image;
image.width = width;
image.height = height;
image.pixels = pixels;

*/


int main(int argc, char **argv){
	//======================= PRE-PROCESSING ===========================//
	//Reads the content of the image file and stores it in an array
	/*TODO :
		- do everything in a cleaner way
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
	unsigned char pic[height][3*width];
	//Create an array of integers instead of binary values
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < 3*width; j+=3) {
			pic[i][j]   = buffer[offset + i*3*width + j];//Red value
			pic[i][j+1] = buffer[offset + i*3*width + j+1];//Green value
			pic[i][j+2] = buffer[offset + i*3*width + j+2];//Blue value
		}
	}

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
		for (int j = 0; j < 3*width; j+=3) {
			newBuffer[width*i*3 + j] = pic[i][j];//Red
			newBuffer[width*i*3 + j + 1] = pic[i][j+1];//Green
			newBuffer[width*i*3 + j + 2] = pic[i][j+2];// Blue
		}
	}
	fwrite(newBuffer, 1, sizeof(newBuffer), pNewFile);
	fclose(pNewFile);

	//======================= END OF PROGRAM ===========================//
	return(0);
}