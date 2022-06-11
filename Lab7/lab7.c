/*
  Tim Driscoll
  Lab 7 ECE 6310
  Motion Tracking
  11/9/2021
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SAMPLETIME 0.05
#define GRAVITY 9.81

int main(int argc, char *argv[])
{
  // Variable Declaration-------------------------------------------------------
  FILE		*fpt, *fpt2;
  //Used to read in text header from the data
  char  h1[320],h2[320],h3[320],h4[320],h5[320],h6[320],h7[320];
  //Used to read in the initial file data
  double time[1300], x_acc[1300], y_acc[1300], z_acc[1300], pitch[1300], roll[1300], yaw[1300];
  //total number of rows of data read
  int totalData;

  //Read in all the data to seperate arrays-------------------------------------
  if ((fpt=fopen("acc_gyro.txt","r")) == NULL)
    {
      printf("Unable to open acc_gyro.txt for reading\n");
      exit(0);
    }
  fscanf(fpt,"%s %s %s %s %s %s %s ",h1,h2,h3,h4,h5,h6,h7); //read in header with lablels (ignore them)
  fpt2 = fopen("acc_gyro_raw.txt","w");
  totalData=0;
  while (fscanf(fpt,"%lf %lf %lf %lf %lf %lf %lf ",&time[totalData],&x_acc[totalData],
  &y_acc[totalData],&z_acc[totalData],&pitch[totalData],&roll[totalData],&yaw[totalData]) == 7)
    {
      fprintf(fpt2,"%lf %lf %lf %lf %lf %lf %lf\n",time[totalData],
      x_acc[totalData],y_acc[totalData],z_acc[totalData],pitch[totalData],roll[totalData],yaw[totalData]);
      totalData++;
    }
  fclose(fpt); //close the text FILE
  fclose(fpt2);

  //Filter (smooth) all the data with a mean filter-----------------------------
  int filterWindow = 15;
  //Used to store filtered data
  double ax[1300], ay[1300], az[1300], gx[1300], gy[1300], gz[1300];
  double sumAX, sumAY, sumAZ, sumGX, sumGY, sumGZ;
  int i,ii;
  for(i = (filterWindow/2); i < (totalData - (filterWindow/2)); i++)
    {
      sumAX = 0;
      sumAY = 0;
      sumAZ = 0;
      sumGX = 0;
      sumGY = 0;
      sumGZ = 0;
      for(ii = (-filterWindow/2); ii <= filterWindow/2; ii++)
        {
          sumAX += x_acc[i+ii];
          sumAY += y_acc[i+ii];
          sumAZ += z_acc[i+ii];
          sumGX += yaw[i+ii];
          sumGY += pitch[i+ii];
          sumGZ += roll[i+ii];
        }
      ax[i] = sumAX/filterWindow;
      ay[i] = sumAY/filterWindow;
      az[i] = sumAZ/filterWindow;
      gx[i] = sumGX/filterWindow;
      gy[i] = sumGY/filterWindow;
      gz[i] = sumGZ/filterWindow;
    }
  //Handle the intial data points and final data points that dont get smoothed
  for(i = (totalData - (filterWindow/2)); i < totalData; i++)
    {
      ax[i] = x_acc[i];
      ay[i] = y_acc[i];
      az[i] = z_acc[i];
      gx[i] = yaw[i];
      gy[i] = pitch[i];
      gz[i] = roll[i];
    }
  for(i = 0; i < (filterWindow/2); i++)
    {
      ax[i] = x_acc[i];
      ay[i] = y_acc[i];
      az[i] = z_acc[i];
      gx[i] = yaw[i];
      gy[i] = pitch[i];
      gz[i] = roll[i];
    }

  fpt = fopen("acc_gyro_smoothed.txt","w");
  for(i = 0; i < totalData; i++)
    {
      //print to terminal
      //printf("%d: %lf %lf %lf %lf %lf %lf %lf\n",i,time[i],
      //ax[i],ay[i],az[i],gx[i],gy[i],gz[i]);
      //print to file
      fprintf(fpt,"%lf %lf %lf %lf %lf %lf %lf\n",time[i],
      ax[i],ay[i],az[i],gx[i],gy[i],gz[i]);
    }
  fclose(fpt);

  //Calculate the variance over defined window----------------------------------
  int varianceWindow = 15;
  //Used to store variance data
  double var_ax[1300], var_ay[1300], var_az[1300], var_gx[1300], var_gy[1300], var_gz[1300];
  double avgAX, avgAY, avgAZ, avgGX, avgGY, avgGZ;
  for(i = (varianceWindow/2); i < (totalData - (varianceWindow/2)); i++)
    {
      sumAX = 0;
      sumAY = 0;
      sumAZ = 0;
      sumGX = 0;
      sumGY = 0;
      sumGZ = 0;
      for(ii = (-varianceWindow/2); ii <= varianceWindow/2; ii++)
        {
          //sum filtered values over window
          sumAX += x_acc[i+ii];
          sumAY += y_acc[i+ii];
          sumAZ += z_acc[i+ii];
          sumGX += yaw[i+ii];
          sumGY += pitch[i+ii];
          sumGZ += roll[i+ii];
        }
      //Calculate the average over window
      avgAX = sumAX/varianceWindow;
      avgAY = sumAY/varianceWindow;
      avgAZ = sumAZ/varianceWindow;
      avgGX = sumGX/varianceWindow;
      avgGY = sumGY/varianceWindow;
      avgGZ = sumGZ/varianceWindow;
      sumAX = 0;
      sumAY = 0;
      sumAZ = 0;
      sumGX = 0;
      sumGY = 0;
      sumGZ = 0;
      for(ii = (-varianceWindow/2); ii <= varianceWindow/2; ii++)
        {
          //sum the difference of the specific point minus the avg then sqaure
          sumAX += pow(x_acc[i+ii]-avgAX,2);
          sumAY += pow(y_acc[i+ii]-avgAY,2);
          sumAZ += pow(z_acc[i+ii]-avgAZ,2);
          sumGX += pow(yaw[i+ii]-avgGX,2);
          sumGY += pow(pitch[i+ii]-avgGY,2);
          sumGZ += pow(roll[i+ii]-avgGZ,2);
        }
      //Divide previous sum by the number of values in the window
      var_ax[i] = sumAX/(varianceWindow-1);
      var_ay[i] = sumAY/(varianceWindow-1);
      var_az[i] = sumAZ/(varianceWindow-1);
      var_gx[i] = sumGX/(varianceWindow-1);
      var_gy[i] = sumGY/(varianceWindow-1);
      var_gz[i] = sumGZ/(varianceWindow-1);

    }
  for(i = 0; i < varianceWindow/2; i++)
    {
      var_ax[i] = 0;
      var_ay[i] = 0;
      var_az[i] = 0;
      var_gx[i] = 0;
      var_gy[i] = 0;
      var_gz[i] = 0;
    }
  for(i = (totalData - (varianceWindow/2)); i < totalData; i++)
    {
      var_ax[i] = 0;
      var_ay[i] = 0;
      var_az[i] = 0;
      var_gx[i] = 0;
      var_gy[i] = 0;
      var_gz[i] = 0;
    }
  //for(i = 0; i < totalData; i++)
//  printf("%d: %lf %lf %lf %lf %lf %lf\n",i,
//  var_ax[i],var_ay[i],var_az[i],var_gx[i],var_gy[i],var_gz[i]);

  //Use previous variances and thresholds to determine when the phone is at rest
  //and when the phone is in motion.
  double startTime, stopTime, integration_a[3], integration_g[3], velocity[3], initialVelocity[3];
  int startIndex, stopIndex, motionCount, j;
  double startRestT, stopRestT;
  int startRestI, stopRestI, restCount;
  int motion = 0;
  double thresholds[] = {0.0008,0.035};

  startRestI = -1;
  startTime = startRestT = stopTime = stopRestI = stopRestT = startIndex = stopIndex = 0;

  restCount = motionCount = 0;
  for(j=0; j < 3; j++)
    {
      integration_a[j] = 0;
      integration_g[j] = 0;
      velocity[j] = 0;
      initialVelocity[j] = 0;
    }

  fpt = fopen("motionCaptures.txt","w");
  //fprintf(fpt,"%s\t%s\t%s\t%s\t%s\t%s\t%s\n-------------------------------------------------------\n",h1,h2,h3,h4,h5,h6,h7);

  for(i = 0; i < totalData; i++)
    {
      //Check to see if variance is greater than defined thresholds
      if(var_ax[i] > thresholds[0] || var_ay[i] > thresholds[0] || var_az[i] > thresholds[0] || var_gx[i] > thresholds[1] || var_gy[i] > thresholds[1] || var_gz[i] > thresholds[1])
          motion = 1;
      else
        motion = 0;

      //Determine if phone is considered to be in motion and a start index has not been defined
      if(motion == 0 && startRestI == -1)
        {
          startRestI = i;
          startRestT = time[i];
        }
      if(motion == 1 && startIndex == 0)
        {
          startIndex = i;
          startTime = time[i];
        }
      //Determine the stop index and stop time
      if((motion == 1 && stopRestI == 0 && startRestI != -1) || i == totalData-1)
        {
          stopRestI = i;
          stopRestT = time[i];
        }
      if(motion == 0 && stopIndex == 0 && startIndex != 0)
        {
          stopIndex = i;
          stopTime = time[i];
        }
      //Check if the time window for motion has been captured
      if(startIndex != 0 && stopIndex != 0)
        {
          motionCount++;
          for(ii = startIndex; ii < stopIndex; ii++)
            {
              //calculate all the rotational distances
              integration_g[0] += yaw[ii] * SAMPLETIME;
              integration_g[1] += pitch[ii] * SAMPLETIME;
              integration_g[2] += roll[ii] * SAMPLETIME;

              //Calculate all the linear distance traveled
              initialVelocity[0] = velocity[0];
              initialVelocity[1] = velocity[1];
              initialVelocity[2] = velocity[2];
              velocity[0] += (x_acc[ii] * GRAVITY * SAMPLETIME);
              velocity[1] += (y_acc[ii] * GRAVITY * SAMPLETIME);
              velocity[2] += (z_acc[ii] * GRAVITY * SAMPLETIME);
              integration_a[0] += (SAMPLETIME * ((initialVelocity[0] + velocity[0])/2));
              integration_a[1] += (SAMPLETIME * ((initialVelocity[1] + velocity[1])/2));
              integration_a[2] += (SAMPLETIME * ((initialVelocity[2] + velocity[2])/2));
              //if(motionCount == 1)
              //  printf("%lf\t%lf\t%lf\n",initialVelocity[2],velocity[2],integration_a[2]);
            }
          //print out all the data to a text file and terminal
          fprintf(fpt,"Motion Capture #%d\nStart Index: %d End Index %d\nStart Time: %lf Stop Time: %lf\n",motionCount,startIndex,stopIndex,startTime,stopTime);
          fprintf(fpt,"Total Angular Rotation\n");
          fprintf(fpt,"Yaw: %lf\tPitch: %lf\tRoll: %lf\n",
          integration_g[0],integration_g[1],integration_g[2]);
          fprintf(fpt,"Total Linear Distance\n");
          fprintf(fpt,"X: %lf\tY: %lf\tZ: %lf\n",
          integration_a[0],integration_a[1],integration_a[2]);
          fprintf(fpt,"-------------------------------------------------------\n");
        /*  fprintf(fpt,"%d & %0.3lf & %0.3lf & %0.3lf & ",motionCount,
          integration_g[0],integration_g[1],integration_g[2]);
          fprintf(fpt,"%0.3lf & %0.3lf & %0.3lf//\n\\hline\n",
          integration_a[0],integration_a[1],integration_a[2]);*/

          startIndex = 0;
          stopIndex = 0;
          startTime = 0;
          stopTime = 0;
          for(j=0; j < 3; j++)
            {
              integration_a[j] = 0;
              integration_g[j] = 0;
              velocity[j] = 0;
              initialVelocity[j] = 0;
            }
        }
      //write out index and time of rest
      if(startRestI != -1 && stopRestI != 0)
        {
          restCount++;

          //print out all the data to a text file and terminal
          fprintf(fpt,"Rest Capture #%d\nStart Index: %d End Index %d\nStart Time: %lf Stop Time: %lf\n",restCount,startRestI,stopRestI,startRestT,stopRestT);
          fprintf(fpt,"-------------------------------------------------------\n");

          startRestI = -1;
          stopRestI = 0;
          startRestT = 0;
          stopRestT = 0;
        }
      motion = 0;
    }
  fclose(fpt);

}
