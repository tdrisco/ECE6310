/*
  Tim Driscoll
  Lab 3 ECE 6310
  Letters
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{
FILE		*fpt,*fpt2, *fptskel;
unsigned char	*Inputimage,*binImage,*msfImage,*Templateimage,desiredLetter;
unsigned char *detectCopy,*thinnedImage,currentLetter, neighbors[8];
char		header[320];
int		ROWS,COLS,BYTES,tempRow,tempCol,letterRow,letterCol,sum,mean,deleteCount;
int r,c,r2,c2,i,T,j,ii,detected,tp,fp,tn,fn,*zeroMean;
float tpr[255],fpr[255],*tempMSFImage,sumMsf,max,min;

//Check for proper command line arguments---------------------------------------
if (argc != 4)
{
printf("Usage: Lab2 [Input Image] [Template Image] [Ground Truth]\n");
exit(0);
}

//------------------------------------------------------------------------------
//Open up the original image and verify-----------------------------------------
if ((fpt=fopen(argv[1],"rb")) == NULL)
{
printf("Unable to open %s for reading\n", argv[1]);
exit(0);
}

//Read in the image header and verify-------------------------------------------
fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
if (strcmp(header,"P5") != 0  ||  BYTES != 255)
{
printf("Not a greyscale 8-bit PPM image\n");
exit(0);
}

//Allocate memory for image data------------------------------------------------
Inputimage=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
tempMSFImage=(float *)calloc(ROWS*COLS,sizeof(float));
msfImage=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
binImage=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
header[0]=fgetc(fpt);	/* read white-space character that separates header */
fread(Inputimage,1,COLS*ROWS,fpt);
fclose(fpt);

//Open up the msf image and verify--------------------------------------------
if ((fpt=fopen(argv[2],"rb")) == NULL)
{
printf("Unable to open %s for reading\n", argv[1]);
exit(0);
}

//Read in the image header and verify-------------------------------------------
fscanf(fpt,"%s %d %d %d",header,&tempCol,&tempRow,&BYTES);
if (strcmp(header,"P5") != 0  ||  BYTES != 255)
{
printf("Not a greyscale 8-bit PPM image\n");
exit(0);
}

//Read in the template image data-----------------------------------------------
Templateimage=(unsigned char *)calloc(tempRow*tempCol,sizeof(unsigned char));
zeroMean=(int *)calloc(tempRow*tempCol,sizeof(int));
header[0]=fgetc(fpt);	/* read white-space character that separates header */
fread(Templateimage,1,tempCol*tempRow,fpt);
fclose(fpt);
//------------------------------------------------------------------------------

//zero mean center the template-------------------------------------------------
for(i = 0; i < (tempRow*tempCol); i++)
    sum += Templateimage[i];
mean = sum/(tempRow*tempCol);
//printf("mean: %d",mean);
for(i = 0; i < (tempRow*tempCol); i++)
  {
    zeroMean[i] = Templateimage[i] - mean;
    //printf("%d\n",zeroMean[i]);
  }
//------------------------------------------------------------------------------
//Find the matched spatial filter-----------------------------------------------
for (r=7; r<ROWS-7; r++)
  for (c=4; c<COLS-4; c++)
    {
      sumMsf=0.0;
      for (r2=-7; r2<=7; r2++)
        for (c2=-4; c2<=4; c2++)
          sumMsf += Inputimage[(r+r2)*COLS+(c+c2)] * zeroMean[(r2+7)*tempCol+(c2+4)];
      tempMSFImage[r*COLS+c]=sumMsf;
    }
//------------------------------------------------------------------------------
//Normalize the matched spatial filter------------------------------------------
max = tempMSFImage[0];
min = tempMSFImage[0];
for(i = 0; i < (ROWS*COLS); i++) //find the max and min
  {
    if(min > tempMSFImage[i])
      min = tempMSFImage[i];
    if(max < tempMSFImage[i])
      max = tempMSFImage[i];
  }
//printf("max:%f\tmin:%f",max,min);
for(i = 0; i < (ROWS*COLS); i++) //normalize msfImage
    msfImage[i] = (255)*((tempMSFImage[i]-min)/(max-min));
//------------------------------------------------------------------------------
//Free any memory that is no longer needed--------------------------------------
free(Templateimage);
free(zeroMean);
free(tempMSFImage);
//------------------------------------------------------------------------------
//Thresholding and classifying the matches--------------------------------------
//open up the ground truth file to be read from
if ((fpt=fopen(argv[3],"r")) == NULL)
  {
    printf("Unable to open %s for reading\n", argv[3]);
    exit(0);
  }

desiredLetter = 'e'; //Letter that is being searched for in the image
detectCopy=(unsigned char *)calloc(tempRow*tempCol,sizeof(unsigned char));
thinnedImage=(unsigned char *)calloc(tempRow*tempCol,sizeof(unsigned char));
unsigned char NW,N,NE,W,E,SW,S,SE;
int edgeToNedge,numNeighbors,condition1,condition2, condition3, endpoint, branchpoint;

//Loop through a range of threshold values from 0-255
for(T = 0; T<=255; T++)
  {
    tp = 0;
    fp =0;
    tn = 0;
    fn =0;
    //threshold the image to create binary Image
    for(j = 0; j < (ROWS*COLS); j++)
      {
        if(msfImage[j] >= T)
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
              if(binImage[(letterRow+r2)*COLS+(letterCol+c2)] == 1)
                detected = 1;
            }

        if(detected == 1)
          {
            //copy detected letter
            ii = 0;
            for (r2=-7; r2<=7; r2++)
            {
              for (c2=-4; c2<=4; c2++)
                {
                  detectCopy[(7+r2)*tempCol+(4+c2)] = Inputimage[(letterRow+r2)*COLS+(letterCol+c2)];
                  ii++;
                }
            //threshold the image at 128
            }

            for(ii = 0; ii < (tempCol*tempRow); ii++)
              {
                if(detectCopy[ii] >= 128)
                  detectCopy[ii] = 0;
                else
                  detectCopy[ii] = 255;
              }

            /*  remove("copy.ppm");
              fptskel=fopen("copy.ppm","w");
              fprintf(fptskel,"P5 9 15 255\n");
              fwrite(detectCopy,135,1,fptskel);
              fclose(fptskel);*/
            //Thin the copied image of the letter
            deleteCount = 1;
            while(deleteCount > 0)
              {
                for(ii = 0; ii < (tempCol*tempRow); ii++)
                  thinnedImage[ii] = 0;

                //printf("here\n");
                deleteCount = 0;
                for (r2=-7; r2<=7; r2++)
                {
                  for (c2=-4; c2<=4; c2++)
                    {
                      NW=0;
                      N=0;
                      NE=0;
                      W=0;
                      E=0;
                      SW=0;
                      S=0;
                      SE= 0;
                      numNeighbors = 0; edgeToNedge = 0; condition1 = 0; condition2 = 0; condition3 = 0;
                      for(j = 0; j < 8; j++)
                        neighbors[j] = 0;
                      if(detectCopy[(7+r2)*tempCol+(4+c2)] == 255)
                        {
                          if((7+r2)==0 || (4+c2)==0)
                            {
                              neighbors[0] = 0;
                              NW = 0;
                            }
                          else
                            {
                              neighbors[0] = detectCopy[(7+r2-1)*tempCol+(4+c2-1)];
                              NW = detectCopy[(7+r2-1)*tempCol+(4+c2-1)];
                            }
                          if((7+r2)==0)
                            {
                              neighbors[1] = 0;
                              N = 0;
                            }
                          else
                            {
                              neighbors[1] = detectCopy[(7+r2-1)*tempCol+(4+c2)];
                              N = detectCopy[(7+r2-1)*tempCol+(4+c2)];
                            }
                          if((7+r2)==0 || (4+c2)==8)
                            {
                              neighbors[2] = 0;
                              NE = 0;
                            }
                          else
                            {
                              neighbors[2] = 0;
                              NE = detectCopy[(7+r2-1)*tempCol+(4+c2+1)];
                            }
                          if((4+c2)==0)
                            {
                              neighbors[7] = 0;
                              W = 0;
                            }
                          else
                            {
                              neighbors[7] = detectCopy[(7+r2)*tempCol+(4+c2-1)];
                              W = detectCopy[(7+r2)*tempCol+(4+c2-1)];
                            }
                          if((4+c2)==8)
                            {
                              neighbors[3] = 0;
                              E = 0;
                            }
                          else
                            {
                              neighbors[3] = detectCopy[(7+r2)*tempCol+(4+c2+1)];
                              E = detectCopy[(7+r2)*tempCol+(4+c2+1)];
                            }
                          if((7+r2)==14 || (4+c2)==0)
                            {
                              neighbors[6] = 0;
                              SW = 0;
                            }
                          else
                            {
                              neighbors[6] = detectCopy[(7+r2+1)*tempCol+(4+c2-1)];
                              SW = detectCopy[(7+r2+1)*tempCol+(4+c2-1)];
                            }
                          if((7+r2)==14)
                            {
                              neighbors[5] = 0;
                              S = 0;
                            }
                          else
                            {
                              neighbors[5] = detectCopy[(7+r2+1)*tempCol+(4+c2)];
                              S = detectCopy[(7+r2+1)*tempCol+(4+c2)];
                            }
                          if((7+r2)==14 || (4+c2)==8)
                            {
                              neighbors[4] = 0;
                              SE = 0;
                            }
                          else
                            {
                              neighbors[4] = detectCopy[(7+r2+1)*tempCol+(4+c2+1)];
                              SE = detectCopy[(7+r2+1)*tempCol+(4+c2+1)];
                            }

                        //now check for edge to non edge transitions
                        for(j = 0; j < 8-1; j++)
                          {
                            if(neighbors[j] == 255 && neighbors[j+1] == 0)
                              edgeToNedge++;
                          }
                        if(neighbors[7] == 255 && neighbors[0] == 0)
                            edgeToNedge++;

                        if(edgeToNedge == 1)
                          condition1 = 1;

                        //Check for the number of neighbors
                        for(j = 0; j < 8; j++)
                          {
                            if(neighbors[j] == 255)
                              numNeighbors++;
                          }
                        if(numNeighbors >= 2 && numNeighbors <= 6)
                          condition2 = 1;

                        //Check for the final condition (specific edge pixels)
                        if(N == 0 || E == 0 || (W == 0 && S == 0))
                          condition3 = 1;

                        if (condition1 == 1 && condition2 == 1 && condition3 == 1)
                          {
                            deleteCount++;
                            thinnedImage[(7+r2)*tempCol+(4+c2)] = 1; // marked for deletion;
                          }

                        }//if statement to check for x pixel in 9x15 copy
                      }//Inner four loop looping through pixels
                    }//outer for loop looping through pixels

                      //Run through all the deletions for this iteration
                      for(j = 0; j < (tempCol*tempRow); j++)
                        {
                          if(thinnedImage[j] == 1)
                          {
                            //printf("here:%d\n",j);
                            detectCopy[j] = 0;
                          }
                        }

                }//while loop to continue thinining image

        endpoint = 0;
        branchpoint = 0;
        for (r2=-7; r2<=7; r2++)
          {
            for (c2=-4; c2<=4; c2++)
              {
                NW=0;
                N=0;
                NE=0;
                W=0;
                E=0;
                SW=0;
                S=0;
                SE = 0;
                edgeToNedge = 0;
                for(j = 0; j < 8; j++)
                  neighbors[j] = 0;
                if(detectCopy[(7+r2)*tempCol+(4+c2)] == 255)
                {
                  //printf("here\n");
                  if((7+r2)==0 || (4+c2)==0)
                    {
                      neighbors[0] = 0;
                      NW = 0;
                    }
                  else
                    {
                      neighbors[0] = detectCopy[(7+r2-1)*tempCol+(4+c2-1)];
                      NW = detectCopy[(7+r2-1)*tempCol+(4+c2-1)];
                    }
                  if((7+r2)==0)
                    {
                      neighbors[1] = 0;
                      N = 0;
                    }
                  else
                    {
                      neighbors[1] = detectCopy[(7+r2-1)*tempCol+(4+c2)];
                      N = detectCopy[(7+r2-1)*tempCol+(4+c2)];
                    }
                  if((7+r2)==0 || (4+c2)==8)
                    {
                      neighbors[2] = 0;
                      NE = 0;
                    }
                  else
                    {
                      neighbors[2] = 0;
                      NE = detectCopy[(7+r2-1)*tempCol+(4+c2+1)];
                    }
                  if((4+c2)==0)
                    {
                      neighbors[7] = 0;
                      W = 0;
                    }
                  else
                    {
                      neighbors[7] = detectCopy[(7+r2)*tempCol+(4+c2-1)];
                      W = detectCopy[(7+r2)*tempCol+(4+c2-1)];
                    }
                  if((4+c2)==8)
                    {
                      neighbors[3] = 0;
                      E = 0;
                    }
                  else
                    {
                      neighbors[3] = detectCopy[(7+r2)*tempCol+(4+c2+1)];
                      E = detectCopy[(7+r2)*tempCol+(4+c2+1)];
                    }
                  if((7+r2)==14 || (4+c2)==0)
                    {
                      neighbors[6] = 0;
                      SW = 0;
                    }
                  else
                    {
                      neighbors[6] = detectCopy[(7+r2+1)*tempCol+(4+c2-1)];
                      SW = detectCopy[(7+r2+1)*tempCol+(4+c2-1)];
                    }
                  if((7+r2)==14)
                    {
                      neighbors[5] = 0;
                      S = 0;
                    }
                  else
                    {
                      neighbors[5] = detectCopy[(7+r2+1)*tempCol+(4+c2)];
                      S = detectCopy[(7+r2+1)*tempCol+(4+c2)];
                    }
                  if((7+r2)==14 || (4+c2)==8)
                    {
                      neighbors[4] = 0;
                      SE = 0;
                    }
                  else
                    {
                      neighbors[4] = detectCopy[(7+r2+1)*tempCol+(4+c2+1)];
                      SE = detectCopy[(7+r2+1)*tempCol+(4+c2+1)];
                    }

                //now check for edge to non edge transitions
                for(j = 0; j < 8-1; j++)
                  {
                    if(neighbors[j] == 255 && neighbors[j+1] == 0)
                      edgeToNedge++;
                  }
                if(neighbors[7] == 255 && neighbors[0] == 0)
                    edgeToNedge++;

                if(edgeToNedge == 1)
                  endpoint++;
                if(edgeToNedge > 2)
                  branchpoint++;
                }
              }
          }



        if(branchpoint == 1 && endpoint == 1)
          detected = 1;
        else
          detected = 0;
      }//if detected in msf
        if(detected == 1 && currentLetter == desiredLetter)
          tp++;
        if(detected == 1 && currentLetter != desiredLetter)
          fp++;
        if(detected == 0 && currentLetter != desiredLetter)
          tn++;
        if(detected == 0 && currentLetter == desiredLetter)
          {
            fn++;
            /*remove("skeleton.ppm");
            fptskel=fopen("skeleton.ppm","w");
            fprintf(fptskel,"P5 9 15 255\n");
            fwrite(detectCopy,135,1,fptskel);
            fclose(fptskel);*/
          }
        }//loop for all letters at given threshold (while)

    tpr[T] = (float)tp/(tp+fn);
    fpr[T] = (float)fp/(fp+tn);
    printf("%d\t%d\t%d\t%f\t%f\n",T,tp,fp,tpr[T],fpr[T]);
    rewind(fpt);
  }// for loop increasing the threshold


fclose(fpt);//Close the ground truth file (no longer needed)

//Output the msf image to a ppm file--------------------------------------------
fpt=fopen("msfImage.ppm","w");
fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
fwrite(msfImage,COLS*ROWS,1,fpt);
fclose(fpt);
fpt=fopen("letter.ppm","w");
fprintf(fpt,"P5 %d %d 255\n",tempCol,tempRow);
fwrite(detectCopy,tempCol*tempRow,1,fpt);
fclose(fpt);
//------------------------------------------------------------------------------

//Print out the values needed to create the ROC curve---------------------------
fpt=fopen("Roc.txt","w");
for(T = 0; T<256; T++)
 {
	fprintf(fpt,"%f %f\n",fpr[T],tpr[T]);
 }
 fclose(fpt);
//------------------------------------------------------------------------------
}
