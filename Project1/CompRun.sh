clear
gcc -o Main Main.c -O3 -lm -std=c99 -fopenmp
./Main lena.ppm 5 25
