
  /*
    Tim Driscoll
    Lab 1 ECE 6310
    Convolution, Separable Filters, Sliding Windows
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[])
{
FILE		*fpt;
unsigned char	*image;
unsigned char	*filtered;
int *seperated;
char		header[320];
char		outputFile[320];
int		ROWS,COLS,BYTES;
int first, last;
int		r,c,r2,c2,sum,option;
struct timespec	tp1,tp2;

if (argc != 3)
  {
  printf("Usage: Lab1 [filename] [Filter Option (1,2, or 3)]\n");
  exit(0);
  }
option = atoi(argv[2]);


//Open up the image and verify
if ((fpt=fopen(argv[1],"rb")) == NULL)
  {
  printf("Unable to open %s for reading\n", argv[1]);
  exit(0);
  }
//Read in the image header and verify
fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
if (strcmp(header,"P5") != 0  ||  BYTES != 255)
  {
  printf("Not a greyscale 8-bit PPM image\n");
  exit(0);
  }
//Allocate memory for image data
image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
header[0]=fgetc(fpt);	/* read white-space character that separates header */
fread(image,1,COLS*ROWS,fpt);
fclose(fpt);


//Allocate memory for the filtered version
filtered = (unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
//Temp array to store all the intermediate values in options 2 and 3
seperated = (int *)calloc(ROWS*COLS,sizeof(int));


//get the start time for speed purposes
clock_gettime(CLOCK_REALTIME,&tp1);
printf("%ld %ld\n",(long int)tp1.tv_sec,tp1.tv_nsec);

//7x7 mean filter where edge cases are dealt with by adding zeros [Option 1]
//This is done automatically because calloc fills arrays with 0
if(option == 1)
  {
    for (r=3; r<ROWS-3; r++)
      for (c=3; c<COLS-3; c++)
        {
          sum=0;
          for (r2=-3; r2<=3; r2++)
            for (c2=-3; c2<=3; c2++)
              sum+=image[(r+r2)*COLS+(c+c2)];
          filtered[r*COLS+c]=sum/49;
        }
  }
//7x7 mean filter seperated into row and column vectors where edge cases are dealt with by adding zeros [Option 2]
//This is done automatically because calloc fills arrays with 0
else if (option == 2)
  {
    //cols mean
    for (r=0; r<ROWS; r++)
      for (c=3; c<COLS-3; c++)
        {
          sum=0;
          for (c2=-3; c2<=3; c2++)
            sum+=image[r*COLS+(c+c2)];
          seperated[r*COLS+c]=sum; //intermidiate array of summed values
        }
      //rows mean
    for (r=3; r<ROWS-3; r++)
      for (c=0; c<COLS; c++)
        {
          sum=0;
          for (r2=-3; r2<=3; r2++)
            sum+=seperated[(r+r2)*COLS+c];
          filtered[r*COLS+c]=sum/49;//final filtered array being averaged
        }
    }
//7x7 mean filter combining seperations and windows where edge cases are dealt with by adding zeros [Option 3]
//This is done automatically because calloc fills arrays with 0
else if(option == 3)
  {
    //cols mean
    for (r=0; r<ROWS; r++)
      {
        for (c=3; c<COLS-3; c++)
          {
            if(c == 3)//checking if its the start of a new row (need to sum all 7 pixels)
              {
                sum = 0;
                for (c2=-3; c2<=3; c2++)
                  sum+=image[r*COLS+(c+c2)];
              }
            else // shifting in the same row meaning we subtract out the first from the previous sum (-4) and add the last from the current window (+3)
              {
                first =image[r*COLS+(c-4)];
                sum -= first;
                last = image[r*COLS+(c+3)];
                sum += last;
              }
          seperated[r*COLS+c]=sum; //intermidiate array of summed values
          }
      }
    //rows mean
    for (c=0; c<COLS; c++)
      {
        for (r=3; r<ROWS-3; r++)
          {
            if(r == 3)//checking if its the start of a new col (need to sum all 7 pixels)
              {
                sum = 0;
                for (r2=-3; r2<=3; r2++)
                  sum+=seperated[(r+r2)*COLS+c];
              }
            else// shifting in the same col meaning we subtract out the first from the previous sum (-4) and add the last from the current window (+3)
              {
                first = seperated[(r-4)*COLS+c];
                sum -= first;
                last = seperated[(r+3)*COLS+c];
                sum += last;
              }
          filtered[r*COLS+c]=sum/49; //final filtered array being averaged
          }
      }
  }
else
  {
    printf("A valid option was not selected in the command line argument\n");
  }
//Get the end time after the filtering has finished
clock_gettime(CLOCK_REALTIME,&tp2);
printf("%ld %ld\n",(long int)tp2.tv_sec,tp2.tv_nsec);

//Print out the time it took for filtering (difference)
printf("%ld\n",tp2.tv_nsec-tp1.tv_nsec);

//Print out the filtered image (create a different image name for each option)
if(option == 1)
  {
    strcpy(outputFile,"filtered_1.ppm");
  }
else if(option == 2)
  {
    strcpy(outputFile,"filtered_2.ppm");
  }
else
  {
    strcpy(outputFile,"filtered_3.ppm");
  }

fpt=fopen(outputFile,"w");
fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
fwrite(filtered,COLS*ROWS,1,fpt);
fclose(fpt);
}
