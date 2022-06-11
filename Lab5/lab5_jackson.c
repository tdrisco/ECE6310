//  Jackson Murrin
//ECE 6310
//  Lab 5

  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <math.h>

  int main (int argc, char *argv[])

  {
  FILE   *fpt;
  int i, COLS, ROWS, BYTES, total_points;
  unsigned char *image, *image_initialpoints;
  char header[80];
  int px[100];
  int py[100];
  int row, col, p, r, c, r2, c2, j, f, n, w, y, q, s, g, u, cnt;
  double *sobel_x, *sobel_y, *sobel_image;
  unsigned char *sobel_normal,*contour_image;
  int max_value,min_value, window, iterations, x_backfirst, y_backfirst, minimum_index;
  double sum;
  double internal_energy1[49], internal_energy2[49], tot, avgdist, external_energy[49];
  double interal_energy1_norm[49],interal_energy2_norm[49],external_energy_norm[49];
  double energy_total[49];


  //Check command line to be correct
  if (argc != 3)
   {
     printf("Usage: Lab 5 [input.ppm] [contour.txt]\n");
     exit(0);
   }
   fpt = fopen(argv[1],"rb");

   //Check ppm file to have contents and complete a few functions
   if (fpt == NULL)
   {
     printf("Unable to find %s for opening\n", argv[1]);
     exit(0);
   }
   i=fscanf(fpt,"%s %d %d %d ", header, &COLS, &ROWS, &BYTES);
   if (i != 4  ||  strcmp(header,"P5") != 0  ||  BYTES != 255)
   {
     printf("%s is not an 8-bit PPM greyscale (P5) image\n", argv[1]);
     fclose(fpt);
     exit(0);
   }
   image = (unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));
   image_initialpoints=(unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));;
   if (image == NULL)
   {
   printf("Unable to allocate %d x %d memory\n",COLS,ROWS);
   exit(0);
   }
   fread(image, 1, ROWS*COLS, fpt);
   fclose(fpt);

  //Check txt file to have contents
   if ((fpt=fopen(argv[2],"rb")) == NULL)
   {
     printf("Unable to open %s for reading\n", argv[2]);
     exit(0);
   }

   total_points=0;
   while (1)
   {
   i=fscanf(fpt, "%d %d", &px[total_points], &py[total_points]);
   if (i != 2)
     break;
   total_points++;
   if (total_points > 100)
     break;
   }
   fclose(fpt);

   for (w=0;w<ROWS*COLS;w++)
   {
     image_initialpoints[w]=image[w];
   }
  //Getting the + onto the image
  for (p=0; p<total_points; p++)
  {
    for (row=py[p]-3; row<=py[p]+3; row++)
    {
      for(col=px[p]-3; col<=px[p]+3; col++)
      {
        if(row==py[p]||col==px[p])
        {
          image_initialpoints[row*COLS+col]=0;
        }
      }
    }
  }

  fpt= fopen("first_hawk_marked.ppm", "w");

  if (fpt == NULL)
    {
    printf("Unable to open %s for reading\n","first_hawk_marked.ppm");
    exit(0);
    }

  fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
  fwrite(image_initialpoints, COLS*ROWS, 1, fpt);
  fclose(fpt);

//Sobel Operator calculations

  int Gx[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
  int Gy[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};

  //convolution
  sobel_x=(double *)calloc(ROWS*COLS, sizeof(double));

   {
     for (r=1; r<ROWS-1; r++)
     {
       for (c=1; c<COLS-1; c++)
         {
           sum=0;
           for (r2=-1; r2<=1; r2++)
           {
             for (c2=-1; c2<=1; c2++)
             {
               sum+=image[(r+r2)*COLS+(c+c2)]*Gx[(r2+1)*3+(c2+1)];
             }
           }
         sobel_x[r*COLS+c]=sum;
         }
     }
   }

   sobel_y=(double *)calloc(ROWS*COLS, sizeof(double));
   {
     for (r=1; r<ROWS-1; r++)
     {
       for (c=1; c<COLS-1; c++)
         {
           sum=0;
           for (r2=-1; r2<=1; r2++)
           {
             for (c2=-1; c2<=1; c2++)
             {
               sum+=image[(r+r2)*COLS+(c+c2)]*Gy[(r2+1)*3+(c2+1)];
             }
           }
         sobel_y[r*COLS+c]=sum;
         }
     }
   }

   //combine 2 sobel convolutions
   sobel_image=(double *)calloc(ROWS*COLS, sizeof(double));
   for(i=0; i<ROWS*COLS; i++)
   {
       sobel_image[i]= sqrt(pow(sobel_x[i],2)+pow(sobel_y[i],2));
    }

   //Normalize combined sobel
   max_value=sobel_image[0];
   min_value=sobel_image[0];
   /*Find min and max values*/
   for (int m=0; m<ROWS*COLS; m++)
   {
     if (max_value<sobel_image[m])
     {
       max_value=sobel_image[m];
     }
   }
   for (int y=0; y<(ROWS*COLS); y++)
   {
     if (min_value>sobel_image[y])
     {
       min_value=sobel_image[y];
     }
   }

   printf("%d,%d\n",max_value,min_value);

   sobel_normal=(unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));
   for(f=0; f<ROWS*COLS; f++)
   {
     //printf("%f\n",sobel_normal[f]);
     sobel_normal[f]=255*((sobel_image[f]-min_value)/(max_value-min_value));
   }

   //Print ppm image
  fpt=fopen("hawk_sobel_image.ppm","w");
  if (fpt == NULL)
    {
      printf("Unable to open %s for reading\n","hawk_sobel_image.ppm");
      exit(0);
    }
  fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
  fwrite(sobel_normal, COLS*ROWS, 1, fpt);
  fclose(fpt);

  // Start Active contour
  window=7;
  iterations=0;
  while(iterations < 1)
  {
    for (cnt = 0;cnt < 1; cnt++)
    {
      //Get internal energy 1
      q=0;
      for(row=(-1*window)/2;row<=window/2;row++)
      {
        for(col=(-1*window)/2;col<= window/2;col++)
        {
          //Checking if the pixel is the last in the row
          if(cnt+1>=total_points)
          {
            internal_energy1[q]=pow(((row+py[cnt])-py[0]),2)+pow(((col+px[cnt])-px[0]),2);
          }
          else
          {
            internal_energy1[q]=pow(((row+py[cnt])-py[cnt+1]),2)+pow(((col+px[cnt])-px[cnt+1]),2);
          }
          q++;
        }
      }

      //Normalize internal energy 1
      max_value=0;
      min_value=0;
      max_value=internal_energy1[0];
      min_value=internal_energy1[0];
      /*Find min and max values*/
      for (int m=0; m<window*window; m++)
      {
        if (max_value<internal_energy1[m])
        {
          max_value=internal_energy1[m];
        }
      }
      for (int y=0; y<window*window; y++)
      {
        if (min_value>internal_energy1[y])
        {
          min_value=internal_energy1[y];
        }
      }
      for(f=0; f<window*window; f++)
      {

        interal_energy1_norm[f]=1*((internal_energy1[f]-min_value)/(max_value-min_value));
        //printf("%f\n",interal_energy1_norm[f]);
      }

      //Get internal energy 2
      s=0;
      tot=0.0;
      for(g=0;g<total_points;g++)
      {
        //checking if pixel is last in Row
        if(g+1>=total_points)
        {
          x_backfirst=px[0];
          y_backfirst=py[0];
        }
        else
        {
          x_backfirst=px[g+1];
          y_backfirst=py[g+1];
        }
        tot+=sqrt(pow((px[g]-x_backfirst),2)+pow((py[g]-y_backfirst),2));
      }
      avgdist=sum/total_points;

      for(row=(-1*window)/2; row<=window/2; row++)
      {
        for(col=(-1*window)/2; col<=window/2; col++)
        {
          if(cnt+1>=total_points)
          {
            internal_energy2[s]=pow(avgdist-sqrt(pow(((row+py[cnt])-py[0]),2)+pow(((col+px[cnt])-px[0]),2)),2);
          }
          else
          {
            internal_energy2[s]=pow(avgdist-sqrt(pow(((row+py[cnt])-py[cnt+1]),2)+pow(((col+px[cnt])-px[cnt+1]),2)),2);;
          }
          s++;
        }
      }

      //Normalize internal energy 2
      max_value=0;
      min_value=0;
      max_value=internal_energy2[0];
      min_value=internal_energy2[0];
      /*Find min and max values*/
      for (int m=0; m<window*window; m++)
      {
        if (max_value<internal_energy2[m])
        {
          max_value=internal_energy2[m];
        }
      }
      for (int y=0; y<window*window; y++)
      {
        if (min_value>internal_energy2[y])
        {
          min_value=internal_energy2[y];
        }
      }
      for(f=0; f<window*window; f++)
      {
        //printf("%f\n",sobel_normal[f]);
        interal_energy2_norm[f]=1*((internal_energy2[f]-min_value)/(max_value-min_value));

      }
      //Get external energy

      u=0;
      for(row=(-1*window)/2; row<=window/2;row++)
      {
        for(col=(-1*window)/2;col<=window/2;col++)
        {
          external_energy[u]=pow(sobel_normal[(row+py[cnt])*COLS+(col+px[cnt])],2);
          u++;
        }
      }
      //Normalize external energy
      max_value=0;
      min_value=0;
      max_value=external_energy[0];
      min_value=external_energy[0];
      /*Find min and max values*/
      for (int m=0; m<window*window; m++)
      {
        if (max_value<external_energy[m])
        {
          max_value=external_energy[m];
        }
      }
      for (int y=0; y<window*window; y++)
      {
        if (min_value>external_energy[y])
        {
          min_value=external_energy[y];
        }
      }
      for(f=0; f<window*window; f++)
      {
        //printf("%f\n",sobel_normal[f]);
        external_energy_norm[f]=1*((external_energy[f]-min_value)/(max_value-min_value));
        printf("%f\n",external_energy_norm[f]);
      }

      //Add 3 energies
      for(i=0;i<window*window;i++)
        {
          energy_total[i]=interal_energy1_norm[i]+interal_energy2_norm[i]-(0.8*external_energy_norm[i]);
        }

      //Find minimum values
      min_value=0;
      min_value=energy_total[0];
      minimum_index=0;
      for (int m=0; m<window*window; m++)
      {
        if (min_value>energy_total[m])
        {
          min_value=energy_total[m];
          minimum_index=m;
        }
        
      }
      px[cnt]=px[cnt]+(minimum_index%window)-(window/2);
      py[cnt]=py[cnt]+(minimum_index/window)-(window/2);
    }
      iterations++;
    }
    //Getting the + onto the image
    contour_image=(unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));
    for (p=0; p<total_points; p++)
    {
      for (row=py[p]-3; row<=py[p]+3; row++)
      {
        for(col=px[p]-3; col<=px[p]+3; col++)
        {
          if(row==py[p]||col==px[p])
          {
            contour_image[row*COLS+col]=0;
          }
        }
      }
    }

  // Print final countoured image
  fpt = fopen("final_contour_hawk.ppm", "w");
  if (fpt == NULL)
  {
    printf("Unable to open %s for writing\n", "final_contour_hawk.ppm");
    exit(0);
  }
  fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
  fwrite(contour_image, ROWS*COLS, 1, fpt);
  fclose(fpt);

  // Final contour points into a text file

  fpt = fopen("final_points.txt","w");
  fprintf(fpt, "Column\t Row\n");
  for (int j = 0; j < total_points; j++)
  {
    fprintf(fpt, "%d\t %d\n", px[j], py[j]);
  }
  fclose(fpt);
 }
