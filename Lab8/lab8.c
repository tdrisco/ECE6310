/*
  Tim Driscoll
  Lab 8 ECE 6310
  Range Image Segmentation
  12/2/2021
*/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define MAX_QUEUE 10000

int main(int argc, char *argv[])
{
  //Variable intializations-----------------------------------------------------
  int	r,c,i,j,threshold,distancePixel;
  int		ROWS,COLS,BYTES;
  char  header[320];
  double	cp[7];
  double	xangle,yangle,dist;
  double	SlantCorrection;
  unsigned char	*RangeImage, *thresholdImage, *grownImage, *colorImage;
  double		P[3][128*128];
  double		SN[3][128*128];
  double	ax,ay,az,bx,by,bz;
  FILE	*fpt;
  //----------------------------------------------------------------------------

  //Read in input image (chair-range.ppm)---------------------------------------
  if (argc != 2)
  {
  printf("Usage: lab8 [Range Image]\n");
  exit(0);
  }

  if ((fpt=fopen(argv[1],"rb")) == NULL)
  {
  printf("Unable to open %s for reading\n", argv[1]);
  exit(0);
  }
  //Read in the image header
  fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
  RangeImage = (unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
  fread(RangeImage,1,COLS*ROWS,fpt);
  fclose(fpt);
  //----------------------------------------------------------------------------

  //Threshold the range image to remove the background--------------------------
  thresholdImage = (unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
  threshold = 130;
  for(i = 0; i < (ROWS*COLS); i++)
  {
    if(RangeImage[i] >= threshold)
    thresholdImage[i] = 255;
    else
    thresholdImage[i] = RangeImage[i];
    //printf("%d\n",thresholdImage[i]);
  }
  if ((fpt=fopen("chair-threshold.ppm","w")) == NULL)
    {
      printf("Unable to open chair-threshold.ppm for writing\n");
      exit(0);
    }
  fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
  fwrite(thresholdImage,1,COLS*ROWS,fpt);
  fclose(fpt);
  //----------------------------------------------------------------------------

  // Caclulate the three dimensional cooridnated of the image-------------------

  cp[0]=1220.7;		/* horizontal mirror angular velocity in rpm */
  cp[1]=32.0;		/* scan time per single pixel in microseconds */
  cp[2]=(COLS/2)-0.5;		/* middle value of columns */
  cp[3]=1220.7/192.0;	/* vertical mirror angular velocity in rpm */
  cp[4]=6.14;		/* scan time (with retrace) per line in milliseconds */
  cp[5]=(ROWS/2)-0.5;		/* middle value of rows */
  cp[6]=10.0;		/* standoff distance in range units (3.66cm per r.u.) */

  cp[0]=cp[0]*3.1415927/30.0;	/* convert rpm to rad/sec */
  cp[3]=cp[3]*3.1415927/30.0;	/* convert rpm to rad/sec */
  cp[0]=2.0*cp[0];		/* beam ang. vel. is twice mirror ang. vel. */
  cp[3]=2.0*cp[3];		/* beam ang. vel. is twice mirror ang. vel. */
  cp[1]/=1000000.0;		/* units are microseconds : 10^-6 */
  cp[4]/=1000.0;			/* units are milliseconds : 10^-3 */

  for (r=0; r<ROWS; r++)
    {
      for (c=0; c<COLS; c++)
        {
          SlantCorrection=cp[3]*cp[1]*((double)c-cp[2]);
          xangle=cp[0]*cp[1]*((double)c-cp[2]);
          yangle=(cp[3]*cp[4]*(cp[5]-(double)r))+(SlantCorrection * 1);//downward direction
          dist=(double)RangeImage[r*COLS+c]+cp[6];
          P[2][r*COLS+c]=sqrt((dist*dist)/(1.0+(tan(xangle)*tan(xangle))
    	     +(tan(yangle)*tan(yangle))));
          P[0][r*COLS+c]=tan(xangle)*P[2][r*COLS+c];
          P[1][r*COLS+c]=tan(yangle)*P[2][r*COLS+c];
        }
    }
  //----------------------------------------------------------------------------

  //Calculate the surface normals-----------------------------------------------
  distancePixel = 3;
  for (r=0; r<ROWS-distancePixel; r++)
    {
      for (c=0; c<COLS-distancePixel; c++)
        {
          //Create a and b vector for cross product
          ax = P[0][r*COLS + (c + distancePixel)] - P[0][r*COLS + c];
          ay = P[1][r*COLS + (c + distancePixel)] - P[1][r*COLS + c];
          az = P[2][r*COLS + (c + distancePixel)] - P[2][r*COLS + c];

          bx = P[0][(r + distancePixel)*COLS + c] - P[0][r*COLS + c];
          by = P[1][(r + distancePixel)*COLS + c] - P[1][r*COLS + c];
          bz = P[2][(r + distancePixel)*COLS + c] - P[2][r*COLS + c];

          SN[0][r*COLS + c] = ay*bz-az*by;
          SN[1][r*COLS + c] = az*bx-ax*bz;
          SN[2][r*COLS + c] = ax*by-ay*bx;
        }
    }
  //----------------------------------------------------------------------------

  //Perform region growing------------------------------------------------------
  int TotalRegions=0;
  int seed = 0;
  int count = 0;
  int regionColor = 40;
  int r2,c2,queue[MAX_QUEUE],qh,qt,index;
  double avgX, avgY, avgZ, totalX, totalY, totalZ, dotProduct,L1,L2,angle;
  grownImage = (unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
  colorImage = (unsigned char *)calloc(3*ROWS*COLS,sizeof(unsigned char));
  for (r=2; r<ROWS-2; r++)
    {
    for (c=2; c<COLS-2; c++)
      {
        //check if pixel is a seed
        seed = 1;
        for(i = -2; i <= 2; i++)
        {
          for(j = -2; j <= 2; j++)
            {
              if(thresholdImage[(i+r)*COLS+(j+c)] == 255 || grownImage[(i+r)*COLS+(j+c)] != 0)
                {
                  seed = 0;
                }
            }
          }
        if(seed == 1)
          {
          //printf("R: %d C: %d\n",r,c);
          TotalRegions++;
          //QueuePaintFill Function---------------------------------------------
          avgX = SN[0][r*COLS+c];
          avgY = SN[1][r*COLS+c];
          avgZ = SN[2][r*COLS+c];

          totalX = SN[0][r*COLS+c];
          totalY = SN[1][r*COLS+c];
          totalZ = SN[2][r*COLS+c];

          count=0;
          queue[0]= r*COLS+c;
          qh= 1;	/* queue head */
          qt= 0;	/* queue tail */
          count = 1;
          while (qt != qh)
            {
            for (r2=-1; r2<=1; r2++)
              for (c2=-1; c2<=1; c2++)
                {
                index = (queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2;
                if (r2 == 0  &&  c2 == 0)
                  continue;
                if ((queue[qt]/COLS+r2) < 0  ||  (queue[qt]/COLS+r2) >= ROWS-distancePixel  || //got removed the minus here
          	  (queue[qt]%COLS+c2) < 0  ||  (queue[qt]%COLS+c2) >= COLS-distancePixel)
                  continue;
                if (grownImage[index] != 0)
                  continue;

                dotProduct = avgX*SN[0][index] + avgY*SN[1][index] + avgZ*SN[2][index];
                L1 = sqrt(pow(avgX,2)+pow(avgY,2)+pow(avgZ,2));
                L2 = sqrt(pow(SN[0][index],2)+pow(SN[1][index],2)+pow(SN[2][index],2));
                angle = acos(dotProduct/(L1 * L2));

                if(angle > 0.8)
                  continue;

                totalX += SN[0][index];
                totalY += SN[1][index];
                totalZ += SN[2][index];

                count++;

                avgX = totalX/count;
                avgY = totalY/count;
                avgZ = totalZ/count;

                grownImage[index] = regionColor;


                queue[qh]=(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2;
                qh=(qh+1)%MAX_QUEUE;
                if (qh == qt)
                  {
                  printf("Max queue size exceeded\n");
                  exit(0);
                  }
                }
            qt=(qt+1)%MAX_QUEUE;
            }
            if(count < 65)
              {
                TotalRegions--;
                int ii;
                for(ii = 0; ii<ROWS*COLS;ii++)
                  {
                    if(grownImage[ii] == regionColor)
                      {
                        grownImage[ii] = 0;
                      }
                  }
              }
            else
              {
                printf("Region labeled %d is %d in size\t\t",TotalRegions,count);
                printf("(%d,%d,%d)\n\n",regionColor*5 % 256,regionColor % 256,regionColor*2 % 256);
                printf("Average surface normal: (%0.3f, %0.3f, %0.3f)\n",avgX,avgY,avgZ);
                regionColor += 40;
              }


          //--------------------------------------------------------------------
          }
      }
    }
  printf("%d total regions were found\n",TotalRegions);
  if ((fpt=fopen("chair-segmented.ppm","w")) == NULL)
    {
      printf("Unable to open chair-segmented.ppm for writing\n");
      exit(0);
    }
  fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
  fwrite(grownImage,1,COLS*ROWS,fpt);
  fclose(fpt);

  if ((fpt=fopen("chair-segmented-color.ppm","w")) == NULL)
    {
      printf("Unable to open chair-segmented.ppm for writing\n");
      exit(0);
    }
  fprintf(fpt,"P6 %d %d 255\n",COLS,ROWS);
  for(i = 0; i < ROWS*COLS; i++)
    {
      colorImage[i*3] = grownImage[i]*5 % 256;
      colorImage[i*3+1] = grownImage[i] % 256;
      colorImage[i*3+2] = grownImage[i]*2 % 256;
    }
  fwrite(colorImage,1,3*COLS*ROWS,fpt);
  fclose(fpt);

  //----------------------------------------------------------------------------
}
