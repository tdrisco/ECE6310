/*
  Tim Driscoll
  Lab 2 ECE 6310
  Optical Character Recognition
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[])
{
FILE		*fpt;
unsigned char	*Inputimage, *Templateimage, *normalImage, *binImage,desiredLetter,currentLetter;
int *zeroMean, sum, mean;
float *msfImage,sumMsf, max, min;
char		header[320];
int		ROWS,COLS,BYTES,tempRow,tempCol,letterRow,letterCol;
int r,c,r2,c2,i,ii,j,detected,tp,fp,tn,fn;
float tpr[255],fpr[255];

if (argc != 4)
{
printf("Usage: Lab2 [Input Image] [Template Image] [Ground Truth]\n");
exit(0);
}

//Open up the input image and verify------------------------------------------
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
Inputimage=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
msfImage=(float *)calloc(ROWS*COLS,sizeof(float));
normalImage=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
binImage=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
header[0]=fgetc(fpt);	/* read white-space character that separates header */
fread(Inputimage,1,COLS*ROWS,fpt);
fclose(fpt);

//Open up the template input image and verify---------------------------------
if ((fpt=fopen(argv[2],"rb")) == NULL)
{
printf("Unable to open %s for reading\n", argv[2]);
exit(0);
}
//Read in the image header and verify
fscanf(fpt,"%s %d %d %d",header,&tempCol,&tempRow,&BYTES);
if (strcmp(header,"P5") != 0  ||  BYTES != 255)
{
printf("Not a greyscale 8-bit PPM image\n");
exit(0);
}
//Allocate memory for image data
Templateimage=(unsigned char *)calloc(tempRow*tempCol,sizeof(unsigned char));
zeroMean=(int *)calloc(tempRow*tempCol,sizeof(int));
header[0]=fgetc(fpt);	/* read white-space character that separates header */
fread(Templateimage,1,tempCol*tempRow,fpt);
fclose(fpt);

//zero mean center the template------------------------------
for(i = 0; i < (tempRow*tempCol); i++)
    sum += Templateimage[i];
mean = sum/(tempRow*tempCol);
//printf("mean: %d",mean);
for(i = 0; i < (tempRow*tempCol); i++)
  {
    zeroMean[i] = Templateimage[i] - mean;
    printf("%d\n",zeroMean[i]);
  }

//Find the matched spatial filter--------------------------------------------
for (r=7; r<ROWS-7; r++)
  for (c=4; c<COLS-4; c++)
    {
      sumMsf=0.0;
      for (r2=-7; r2<=7; r2++)
        for (c2=-4; c2<=4; c2++)
          sumMsf += Inputimage[(r+r2)*COLS+(c+c2)] * zeroMean[(r2+7)*tempCol+(c2+4)];
      msfImage[r*COLS+c]=sumMsf;
      //printf("%f\n",msfImage[r*COLS+c]);
    }
//Normalize the matched spatial filter-----------------------------------------
max = msfImage[0];
min = msfImage[0];
for(i = 0; i < (ROWS*COLS); i++) //find the max and min
  {
    if(min > msfImage[i])
      min = msfImage[i];
    if(max < msfImage[i])
      max = msfImage[i];
  }
printf("max:%f\tmin:%f",max,min);
for(i = 0; i < (ROWS*COLS); i++) //normalize msfImage
  {
    normalImage[i] = (255)*((msfImage[i]-min)/(max-min));
    //printf("%d\n",normalImage[i]);
  }
//open up the ground truth file to be read from
if ((fpt=fopen(argv[3],"r")) == NULL)
  {
    printf("Unable to open %s for reading\n", argv[3]);
    exit(0);
  }
free(Templateimage);
free(zeroMean);
free(Inputimage);
free(msfImage);
//tpr=(float *)calloc(255,sizeof(float));
//fpr=(float *)calloc(255,sizeof(float));
//loop through a range of t
desiredLetter = 'e';
for(ii = 0; ii<=255; ii++)
  {
    tp = 0;
    fp =0;
    tn = 0;
    fn =0;
    //threshold the image to create binary Image
    for(j = 0; j < (ROWS*COLS); j++)
      {
        if(normalImage[j] >= ii)
          binImage[j] = 1;
        else
          binImage[j] = 0;
      }
    //loop through the ground Truth
    while (fscanf(fpt,"%c %d %d ",&currentLetter,&letterCol,&letterRow) != EOF)
      {
        detected = 0;

        for (r2=-7; r2<=7; r2++)
          for (c2=-4; c2<=4; c2++)
            {
              //printf("%d\n",normalImage[(letterRow+r2)*COLS+(letterCol+c2)]);
              if(binImage[(letterRow+r2)*COLS+(letterCol+c2)] == 1)
                detected = 1;
            }

        if(detected == 1 && currentLetter == desiredLetter)
          tp++;
        if(detected == 1 && currentLetter != desiredLetter)
          fp++;
        if(detected == 0 && currentLetter != desiredLetter)
          tn++;
        if(detected == 0 && currentLetter == desiredLetter)
          fn++;
      }
    //printf("T\tTP\tFP\n");
    tpr[ii] = (float)tp/(tp+fn);
    fpr[ii] = (float)fp/(fp+tn);
    printf("%d\t%d\t%d\t%f\t%f\n",ii,tp,fp,tpr[ii],fpr[ii]);
    rewind(fpt);
  }
fclose(fpt);
fpt=fopen("outputFile.ppm","w");
fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
fwrite(normalImage,COLS*ROWS,1,fpt);
fclose(fpt);


fpt=fopen("Roc.txt","w");
for(ii = 0; ii<256; ii++)
 {
	fprintf(fpt,"%f %f\n",fpr[ii],tpr[ii]);
 }


fclose(fpt);
}
