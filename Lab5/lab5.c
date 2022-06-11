/*
  Tim Driscoll
  Lab 5 ECE 6310
  Active Contours
  10/28/21
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char *argv[])
{
  FILE		*fpt;
  int		ROWS,COLS,BYTES;
  char  header[320];
  unsigned char *inputImage, *initialContour;
  int yPoints[60], xPoints[60], totalData;


  if (argc != 3)
  {
  printf("Usage: Lab5 [Initial Contour Image] [Final Contour Image]\n");
  exit(0);
  }

  //Open up the input image and verify------------------------------------------
  if ((fpt=fopen("hawk.ppm","rb")) == NULL)
  {
  printf("Unable to open hawk.ppm for reading\n");
  exit(0);
  }

  //Read in the image header and verify
  fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
  if (strcmp(header,"P5") != 0  ||  BYTES != 255)
  {
  printf("Not a greyscale 8-bit PPM image\n");
  exit(0);
  }

  //allocate memory and read in input Image
  inputImage=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
  initialContour=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
  header[0]=fgetc(fpt);	/* read white-space character that separates header */
  fread(inputImage,1,COLS*ROWS,fpt);
  fclose(fpt);

  int i;
  for(i = 0; i < COLS*ROWS; i++)
    {
      initialContour[i] = inputImage[i];
    }

  //Open and read in the intial contour points (y,x)
  if ((fpt=fopen("hawk_init.txt","r")) == NULL)
  {
  printf("Unable to open hawk_init.txt for reading\n");
  exit(0);
  }
  totalData=0;
  while (fscanf(fpt,"%d %d",&xPoints[totalData], &yPoints[totalData]) == 2)
    {
    //printf("%d %d\n",xPoints[totalData],yPoints[totalData]);
    totalData++;
    }
  fclose(fpt);
  //printf("\n%d\n",totalData);
  //Plot the intial contour points on the input image
  //copy detected letter
  int r,c;

  for(i = 0; i < totalData; i++)
    {
        for(r=-3; r<=3; r++)
          initialContour[(yPoints[i]+r)*COLS+(xPoints[i])] = 0;
        for(c=-3; c<=3; c++)
          initialContour[(yPoints[i])*COLS+(xPoints[i]+c)] = 0;
    }

  //Output the intial contour of the image
  if ((fpt=fopen(argv[1],"w")) == NULL)
  {
  printf("Unable to open %s for reading\n", argv[1]);
  exit(0);
  }
  fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
  fwrite(initialContour,COLS*ROWS,1,fpt);
  fclose(fpt);

  //Use sobel template to filter the Image
  int r2,c2;
  double sobelValX, sobelValY, *sobel, max, min;
  unsigned char  *normalizedSobel;
  int gx[9] = {1,0,-1,2,0,-2,1,0,-1};
  int gy[9] = {1,2,1,0,0,0,-1,-2,-1};
  sobel=(double *)calloc(ROWS*COLS,sizeof(double));
  normalizedSobel = (unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));

  for (r=1; r<ROWS-1; r++)
    {
      for (c=1; c<COLS-1; c++)
        {
          sobelValX=0.0;
          sobelValY=0.0;
          for (r2=-1; r2<=1; r2++)
            {
              for (c2=-1; c2<=1; c2++)
                {
                  sobelValX += inputImage[(r+r2)*COLS+(c+c2)] * gx[(r2+1)*3+(c2+1)];
                  sobelValY += inputImage[(r+r2)*COLS+(c+c2)] * gy[(r2+1)*3+(c2+1)];
                }
            }
          sobel[r*COLS+c]= sqrt(pow(sobelValX,2) + pow(sobelValY,2));
        }
    }

  //Normalize the sobel filtered image
  max = sobel[0];
  min = sobel[0];
  for(i = 0; i < (ROWS*COLS); i++) //find the max and min
    {
      if(min > sobel[i])
        min = sobel[i];
      if(max < sobel[i])
        max = sobel[i];
    }
  printf("max: %f\tmin: %f",max,min);
  for(i = 0; i < (ROWS*COLS); i++) //normalize msfImage
    {
      normalizedSobel[i] = (255)*((sobel[i]-min)/(max-min));
      //printf("%f\n",(255)*((sobel[i]-min)/(max-min)));
    }

  //Output the intial contour of the image
  if ((fpt=fopen("sobel_hawk.ppm","wb")) == NULL)
    {
      printf("Unable to open sobel_hawk.ppm for writing\n");
      exit(0);
    }
  fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
  fwrite(normalizedSobel,COLS*ROWS,1,fpt);
  fclose(fpt);

  //Begin the process of active contouring
  int iterations = 1;
  int window = 7;
  double internalE1[window*window], internalE2[window*window], externalE[window*window];
  double norminternalE1[window*window], norminternalE2[window*window], normexternalE[window*window];
  double sumDistance, avgDistance, totalEnergy[window*window];
  int j,E1count,E2count,E3count,ii,jj,minIndex, iii;
  for(ii = 0; ii < iterations; ii++) //run for 30 total iterations
    {
      sumDistance = 0;
      for(j = 0; j < 1; j++)
        {
          //printf("here %d\n",j);
          //Calculate the first internal energy
          E1count = 0;
          for (r= (-window/2); r <= window/2; r++)
            {
              for (c= (-window/2); c <= window/2; c++)
                {
                  if(j == totalData-1)
                    internalE1[E1count] = pow(r+yPoints[j]-yPoints[0],2) + pow(c+xPoints[j]-xPoints[0],2);
                  else
                    internalE1[E1count] = pow(r+yPoints[j]-yPoints[j+1],2) + pow(c+xPoints[j]-xPoints[j+1],2);
                  E1count++;
                }
            }

          //Calculate the second internal energy
          E2count = 0;
          sumDistance = 0.0;
          for(iii = 0; iii < totalData; iii++)
            {
              if(iii == totalData-1)
                {
                  sumDistance += sqrt(pow(yPoints[iii]-yPoints[0],2) + pow(xPoints[iii]-xPoints[0],2));
                }

              else
                {
                  sumDistance += sqrt(pow(yPoints[iii]-yPoints[iii+1],2) + pow(xPoints[iii]-xPoints[iii+1],2));
                }

            }
          avgDistance = sumDistance/totalData;
          //printf("\n%f\n",avgDistance);
          for (r= (-window/2); r <=window/2; r++)
            {
              for (c= (-window/2); c <=window/2; c++)
                {
                  if(j == totalData-1)
                    internalE2[E2count] = pow(avgDistance - sqrt(pow(r+yPoints[j]-yPoints[0],2) + pow(c+xPoints[j]-xPoints[0],2)),2);
                  else
                    internalE2[E2count] = pow(avgDistance - sqrt(pow(r+yPoints[j]-yPoints[j+1],2) + pow(c+xPoints[j]-xPoints[j+1],2)),2);
                  E2count++;
                }
            }

          //Calculate the external energy
          E3count = 0;
          for (r= (-window/2); r <=window/2; r++)
            {
              for (c= (-window/2); c <=window/2; c++)
                {
                  externalE[E3count] = pow(normalizedSobel[(r+yPoints[j])*COLS+(c+xPoints[j])],2);
                  E3count++;
                }
            }
          //Normalize all three of the energies
          max = internalE1[0];
          min = internalE1[0];
          for(i = 0; i < (window*window); i++) //find the max and min
            {
              if(min > internalE1[i])
                min = internalE1[i];
              if(max < internalE1[i])
                max = internalE1[i];
            }
          for(i = 0; i < (window*window); i++)
            {
              norminternalE1[i] = (1)*((internalE1[i]-min)/(max-min));
            }
          max = internalE2[0];
          min = internalE2[0];
          for(i = 0; i < (window*window); i++) //find the max and min
            {
              if(min > internalE2[i])
                min = internalE2[i];
              if(max < internalE2[i])
                max = internalE2[i];
            }
          for(i = 0; i < (window*window); i++)
            {
              norminternalE2[i] = (1)*((internalE2[i]-min)/(max-min));
            }
          max = externalE[0];
          min = externalE[0];
          for(i = 0; i < (window*window); i++) //find the max and min
            {
              if(min > externalE[i])
                min = externalE[i];
              if(max < externalE[i])
                max = externalE[i];
            }
          for(i = 0; i < (window*window); i++)
            {
              normexternalE[i] = (1)*((externalE[i]-min)/(max-min));
              //printf("%f\n",normexternalE[i]);
            }
          //Add up all the final energies
          for (jj= 0; jj < window*window; jj++)
            {
              totalEnergy[jj] = norminternalE1[jj] + norminternalE2[jj] - (0.8*normexternalE[jj]);
            }
          //find the min energy location
          min = totalEnergy[0];
          minIndex = 0;
          for (jj= 0; jj < window*window; jj++)
            {
              if(min > totalEnergy[jj])
                {
                  min = totalEnergy[jj];
                  minIndex = jj;
                }
            }
          //Update the contour point location (least energy)
          xPoints[j] = xPoints[j] + (minIndex % window) - (window / 2);
          yPoints[j] = yPoints[j] + (minIndex / window) - (window / 2);
      }//totalData loop
    }//Iteration loop


  fpt = fopen("hawk_finalPoints.txt","wb");
  for(i = 0; i < totalData; i++)
    {
        printf("%d %d\n",xPoints[i],yPoints[i]);
        fprintf(fpt, "%d %d\n", xPoints[i], yPoints[i]);
        for(r=-3; r<=3; r++)
          inputImage[(yPoints[i]+r)*COLS+(xPoints[i])] = 0;
        for(c=-3; c<=3; c++)
          inputImage[(yPoints[i])*COLS+(xPoints[i]+c)] = 0;
    }
  fclose(fpt);
  //Output the intial contour of the image
  if ((fpt=fopen(argv[2],"w")) == NULL)
    {
      printf("Unable to open %s for reading\n", argv[2]);
      exit(0);
    }
  fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
  fwrite(inputImage,COLS*ROWS,1,fpt);
  fclose(fpt);

}
