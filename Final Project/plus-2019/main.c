
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <process.h>	/* needed for multithreading */
#include "resource.h"
#include "globals.h"


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				LPTSTR lpCmdLine, int nCmdShow)

{
MSG			msg;
HWND		hWnd;
WNDCLASS	wc;

wc.style=CS_HREDRAW | CS_VREDRAW;
wc.lpfnWndProc=(WNDPROC)WndProc;
wc.cbClsExtra=0;
wc.cbWndExtra=0;
wc.hInstance=hInstance;
wc.hIcon=LoadIcon(hInstance,"ID_PLUS_ICON");
wc.hCursor=LoadCursor(NULL,IDC_ARROW);
wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
wc.lpszMenuName="ID_MAIN_MENU";
wc.lpszClassName="PLUS";

if (!RegisterClass(&wc))
  return(FALSE);

hWnd=CreateWindow("PLUS","plus program",
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
		CW_USEDEFAULT,0,400,400,NULL,NULL,hInstance,NULL);
if (!hWnd)
  return(FALSE);

ShowScrollBar(hWnd,SB_BOTH,FALSE);
ShowWindow(hWnd,nCmdShow);
UpdateWindow(hWnd);
MainWnd=hWnd;

//Intialize all the switches to be zero
ShowPixelCoords=0;
BigDots = 0;
Original_switch = 0;
Sobel_switch = 0;
RubberBand_switch = 0;
Balloon_switch = 0;
rubberbandCount = 0;
rbIndex = 0;
balloonIndex = 0;
Neutral_switch = 0;
pointMove = 0;
match = -1;

strcpy(filename,"");
OriginalImage=NULL;
ROWS=COLS=0;

InvalidateRect(hWnd,NULL,TRUE);
UpdateWindow(hWnd);

while (GetMessage(&msg,NULL,0,0))
  {
  TranslateMessage(&msg);
  DispatchMessage(&msg);
  }
return(msg.wParam);
}




LRESULT CALLBACK WndProc (HWND hWnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam)

{
HMENU				hMenu;
OPENFILENAME		ofn;
FILE				*fpt;
HDC					hDC;
char				header[320],text[320];
int					BYTES,xPos,yPos,x,y;

switch (uMsg)
  {
  case WM_COMMAND:
    switch (LOWORD(wParam))
      {
	  case ID_FILE_LOAD:
		Original_switch = 1;
		Sobel_switch = 0;
		if (OriginalImage != NULL)
		  {
		  free(OriginalImage);
		  OriginalImage=NULL;
		  }
		memset(&(ofn),0,sizeof(ofn));
		ofn.lStructSize=sizeof(ofn);
		ofn.lpstrFile=filename;
		filename[0]=0;
		ofn.nMaxFile=MAX_FILENAME_CHARS;
		ofn.Flags=OFN_EXPLORER | OFN_HIDEREADONLY;
		ofn.lpstrFilter = "PNM files\0*.pnm\0All files\0*.*\0\0";
		if (!( GetOpenFileName(&ofn))  ||  filename[0] == '\0')
		  break;		/* user cancelled load */
		if ((fpt=fopen(filename,"rb")) == NULL)
		  {
		  MessageBox(NULL,"Unable to open file",filename,MB_OK | MB_APPLMODAL);
		  break;
		  }
		fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
		if (strcmp(header,"P6") != 0  ||  BYTES != 255)
		  {
		  MessageBox(NULL,"Not a PPM (P6 color) image",filename,MB_OK | MB_APPLMODAL);
		  fclose(fpt);
		  break;
		  }
		OriginalImage=(unsigned char *)calloc(3*ROWS*COLS,1); //size of rgb
		//header[0]=fgetc(fpt);	/* whitespace character after header */
		fread(OriginalImage,1,3*ROWS*COLS,fpt);
		fclose(fpt);
		CalculateGreyScale();
		CalculateSobelImage();
		SetWindowText(hWnd,filename);
		PaintImage();
		break;

      case ID_FILE_QUIT:
        DestroyWindow(hWnd);
        break;

	  case ID_DISPLAY_ORIGINALIMAGE:
		Original_switch = 1;
		Sobel_switch = 0;
		PaintImage();
		break;

	  case ID_DISPLAY_CLEARIMAGE:
		  Original_switch = 1;
		  Sobel_switch = 0;
		  PaintImage();
		  break;

	  case ID_DISPLAY_SOBELIMAGE:
		Original_switch = 0;
		Sobel_switch = 1;
		PaintSobel();
		break;

	  case ID_CONTOUR_RUBBERBANDMODEL:
		  RubberBand_switch = (RubberBand_switch + 1) % 2;
		  if (RubberBand_switch)
		  {
			  rubberbandCount = 0;
			  rbIndex = 0;
		  }
		break;

	  case ID_CONTOUR_BALLOONMODEL:
		  Balloon_switch = (Balloon_switch + 1) % 2;
		  if (Balloon_switch)
			  balloonIndex = 0;
		  break;
	  case ID_CLEAR_CLEARCOUNTOUR:
		  balloonIndex = 0;
		  rubberbandCount = 0;
		  rbIndex = 0;
		  balloonIndex = 0;
		  PaintImage();
		  break;

	  case ID_CONTOUR_NEUTRALMODEL:
		  if (Neutral_switch)
			  Neutral_switch = 0;
		  else if (Balloon_switch == 0 && RubberBand_switch == 0)
			  MessageBox(NULL, "Must previously complete a balloon or rubberband contour!", "Neutral Model", MB_OK | MB_APPLMODAL);
		  else if (Balloon_switch)
		  {
			  Neutral_switch = (Neutral_switch + 1) % 2;
			  for (int i = 0; i < balloonIndex; i++)
			  {
				  //copy over the final contour points
				  neutralX[i] = balloonX[i];
				  neutralY[i] = balloonY[i];
			  }
			  neutralIndex = balloonIndex/3;
			  Balloon_switch = 0;
		  }
		  else if (RubberBand_switch)
		  {
			  Neutral_switch = (Neutral_switch + 1) % 2;
			  for (int i = 0; i < rbIndex; i++)
			  {
				  //copy over the final contour points
				  neutralX[i] = rubberbandX[i];
				  neutralY[i] = rubberbandY[i];
			  }
			  neutralIndex = rbIndex;
			  RubberBand_switch = 0;
		  }
		  break;
      }
    break;
  case WM_SIZE:		  /* could be used to detect when window size changes */
	  if (Original_switch)
		  PaintImage();
	  else
		  PaintSobel();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_PAINT:
	  if (Original_switch)
		  PaintImage();
	  else
		  PaintSobel();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_LBUTTONDOWN:
	  shiftKey = GetKeyState(VK_SHIFT);
	  if (shiftKey != 1 && match >= 0 && Neutral_switch)
	  {
		  pointMove = 1;
	  }
	  else
	  {
		  pointMove = 0;
	  }
	  return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_RBUTTONDOWN:
	  mouse_x = LOWORD(lParam);
	  mouse_y = HIWORD(lParam);
	  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	  break;
  case WM_MOUSEMOVE:
	if (RubberBand_switch && (wParam & WM_LBUTTONDOWN) && !Neutral_switch)
	  {
	  xPos=LOWORD(lParam);
	  yPos=HIWORD(lParam);
	  if (xPos >= 0  &&  xPos < COLS  &&  yPos >= 0  &&  yPos < ROWS)
		{
		  if (rubberbandCount % 5 == 0)
			{
			  rubberbandX[rbIndex] = xPos;
			  rubberbandY[rbIndex] = yPos;
			  rbIndex++;
		    }
		rubberbandCount++;
		sprintf(text,"%d,%d=>%d     ",xPos,yPos,OriginalImage[yPos*COLS+xPos]);
		hDC=GetDC(MainWnd);
		TextOut(hDC,0,0,text,strlen(text));		/* draw text on the window */
		for (x=-2; x<=2; x++)
			for (y=-2; y<=2; y++)
			  SetPixel(hDC, xPos+x, yPos+y, RGB(255, 0, 0));
		ReleaseDC(MainWnd,hDC);
		}
	  }
	if (Neutral_switch)
	{
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		hDC = GetDC(MainWnd);
		for (int i = 0; i < neutralIndex; i++)
		{
			if ((neutralX[i] >= xPos-2 && neutralX[i] <= xPos+2) && (neutralY[i] >= yPos-2 && neutralY[i] <= yPos+2))
			{
				sprintf(text, "%d,%d=>Match     ", xPos, yPos);
				SetTextColor(hDC, RGB(0, 255, 0));
				//shiftKey = GetKeyState(VK_SHIFT);
				//if(shiftKey != 1 && (wParam & WM_LBUTTONDOWN))
				match = i;
				break;
			}	
			else
			{
				sprintf(text, "%d,%d=>No Match     ", xPos, yPos);
				SetTextColor(hDC, RGB(255, 0, 0));
				if (pointMove == 0)
				{
					match = -1;
				}
			}
				
		}	
		TextOut(hDC, 0, 0, text, strlen(text));
		ReleaseDC(MainWnd, hDC);	
	}
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;

  case WM_LBUTTONUP:
	  if (RubberBand_switch)
	  {
		  //This is where I will start the thread for contouring
		  PaintImage();
		  for (int i = 0; i < rbIndex;i++)
		  {
			  hDC = GetDC(MainWnd);
			  for (x = -2; x <= 2; x++)
				  for (y = -2; y <= 2; y++)
					  SetPixel(hDC, rubberbandX[i] + x, rubberbandY[i] + y, RGB(0, 0, 255));
			  ReleaseDC(MainWnd, hDC);
		  }
		  _beginthread(RubberBandThread, 0, MainWnd);
	  }
	  if (pointMove)
	  {
		  neutralX[match] = LOWORD(lParam);
		  neutralY[match] = HIWORD(lParam);
		  PaintImage();
		  hDC = GetDC(MainWnd);
		  for (int i = 0; i < neutralIndex; i++)
		  {
			  for (x = -2; x <= 2; x++)
				  for (y = -2; y <= 2; y++)
					  if (i == match)
						  SetPixel(hDC, neutralX[i] + x, neutralY[i] + y, RGB(255, 0, 255));
					  else
						  SetPixel(hDC, neutralX[i] + x, neutralY[i] + y, RGB(0, 0, 255));
			  
		  }
		  ReleaseDC(MainWnd, hDC);
		  _beginthread(NeutralThread, 0, MainWnd);
	  }
	  
	return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	break;

  case WM_RBUTTONUP:
	  if (Balloon_switch)
	  {
		  xPos = LOWORD(lParam);
		  yPos = HIWORD(lParam);
		  int i = 0;
		  hDC = GetDC(MainWnd);
		  if (xPos >= 0 && xPos < COLS && yPos >= 0 && yPos < ROWS)
		  {
			  for (double j = 0; j < 2 * M_PI; j += 0.1)
			  {
				  
					  balloonX[balloonIndex] = (int)(xPos + 10 * cos(j));
					  balloonY[balloonIndex] = (int)(yPos + 10 * sin(j));
					  balloonIndex++;
				  for (x = -2; x <= 2; x++)
					  for (y = -2; y <= 2; y++)
						SetPixel(hDC,(int)(xPos + 10 * cos(j))+x, (int)(yPos + 10 * sin(j))+y, RGB(255, 0, 0));
				  //ReleaseDC(MainWnd, hDC);
			  }
		  }	
		  _beginthread(BalloonThread, 0, MainWnd);
	  }
	  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	  break;

  case WM_KEYDOWN:
	if (wParam == 's'  ||  wParam == 'S')
	  PostMessage(MainWnd,WM_COMMAND,ID_SHOWPIXELCOORDS,0);	  /* send message to self */
	return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_TIMER:	  /* this event gets triggered every time the timer goes off */
	hDC=GetDC(MainWnd);
	SetPixel(hDC,TimerCol,TimerRow,RGB(0,0,255));	/* color the animation pixel blue */
	ReleaseDC(MainWnd,hDC);
	TimerRow++;
	TimerCol+=2;
	break;
  case WM_HSCROLL:	  /* this event could be used to change what part of the image to draw */
	PaintImage();	  /* direct PaintImage calls eliminate flicker; the alternative is InvalidateRect(hWnd,NULL,TRUE); UpdateWindow(hWnd); */
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_VSCROLL:	  /* this event could be used to change what part of the image to draw */
	PaintImage();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
    break;
  }

hMenu=GetMenu(MainWnd);
if (Sobel_switch == 1)
  CheckMenuItem(hMenu,ID_DISPLAY_SOBELIMAGE,MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
else
  CheckMenuItem(hMenu, ID_DISPLAY_SOBELIMAGE,MF_UNCHECKED);
if (Original_switch == 1)
CheckMenuItem(hMenu, ID_DISPLAY_ORIGINALIMAGE, MF_CHECKED);	
else
CheckMenuItem(hMenu, ID_DISPLAY_ORIGINALIMAGE, MF_UNCHECKED);
if (RubberBand_switch == 1)
CheckMenuItem(hMenu, ID_CONTOUR_RUBBERBANDMODEL, MF_CHECKED);
else
CheckMenuItem(hMenu, ID_CONTOUR_RUBBERBANDMODEL, MF_UNCHECKED);
if (Balloon_switch == 1)
CheckMenuItem(hMenu, ID_CONTOUR_BALLOONMODEL, MF_CHECKED);
else
CheckMenuItem(hMenu, ID_CONTOUR_BALLOONMODEL, MF_UNCHECKED);
if (Neutral_switch == 1)
CheckMenuItem(hMenu, ID_CONTOUR_NEUTRALMODEL, MF_CHECKED);
else
CheckMenuItem(hMenu, ID_CONTOUR_NEUTRALMODEL, MF_UNCHECKED);

DrawMenuBar(hWnd);

return(0L);
}




void PaintImage()

{
PAINTSTRUCT			Painter;
HDC					hDC;
BITMAPINFOHEADER	bm_info_header;
BITMAPINFO			*bm_info;
int					i,r,c,DISPLAY_ROWS,DISPLAY_COLS;
unsigned char		*DisplayImage;

if (OriginalImage == NULL)
  return;		/* no image to draw */

		/* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
DISPLAY_ROWS=ROWS;
DISPLAY_COLS=COLS;
if (DISPLAY_ROWS % 4 != 0)
  DISPLAY_ROWS=(DISPLAY_ROWS/4+1)*4;
if (DISPLAY_COLS % 4 != 0)
  DISPLAY_COLS=(DISPLAY_COLS/4+1)*4;
DisplayImage=(unsigned char *)calloc(DISPLAY_ROWS*DISPLAY_COLS,1);
for (r=0; r<ROWS; r++)
  for (c=0; c<COLS; c++)
	DisplayImage[r*DISPLAY_COLS+c]= greyScale[r * COLS + c];

BeginPaint(MainWnd,&Painter);
hDC=GetDC(MainWnd);
bm_info_header.biSize=sizeof(BITMAPINFOHEADER); 
bm_info_header.biWidth=DISPLAY_COLS;
bm_info_header.biHeight=-DISPLAY_ROWS; 
bm_info_header.biPlanes=1;
bm_info_header.biBitCount=8; 
bm_info_header.biCompression=BI_RGB; 
bm_info_header.biSizeImage=0; 
bm_info_header.biXPelsPerMeter=0; 
bm_info_header.biYPelsPerMeter=0;
bm_info_header.biClrUsed=256;
bm_info_header.biClrImportant=256;
bm_info=(BITMAPINFO *)calloc(1,sizeof(BITMAPINFO) + 256*sizeof(RGBQUAD));
bm_info->bmiHeader=bm_info_header;
for (i=0; i<256; i++)
  {
  bm_info->bmiColors[i].rgbBlue=bm_info->bmiColors[i].rgbGreen=bm_info->bmiColors[i].rgbRed=i;
  bm_info->bmiColors[i].rgbReserved=0;
  } 

SetDIBitsToDevice(hDC,0,0,DISPLAY_COLS,DISPLAY_ROWS,0,0,
			  0, /* first scan line */
			  DISPLAY_ROWS, /* number of scan lines */
			  DisplayImage,bm_info,DIB_RGB_COLORS);
ReleaseDC(MainWnd,hDC);
EndPaint(MainWnd,&Painter);

free(DisplayImage);
free(bm_info);
}

void CalculateGreyScale()
{
	int r, c;
	greyScale = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
	for (r = 0; r < ROWS; r++)
		for (c = 0; c < COLS; c++)
			greyScale[r * COLS + c] = (int)((OriginalImage[(r * COLS + c) * 3] + OriginalImage[(r * COLS + c) * 3 + 1] + OriginalImage[(r * COLS + c) * 3 + 2]) / 3);
}

void CalculateSobelImage()
{
	//Use sobel template to filter the Image
	int r2, c2,r,c,i;
	double sobelValX, sobelValY, * sobel, max, min;
	unsigned char* normalizedSobel;
	int gx[9] = { 1,0,-1,2,0,-2,1,0,-1 };
	int gy[9] = { 1,2,1,0,0,0,-1,-2,-1 };
	sobel = (double*)calloc(ROWS * COLS, sizeof(double));
	sobelImage = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));

	for (r = 1; r < ROWS - 1; r++)
	{
		for (c = 1; c < COLS - 1; c++)
		{
			sobelValX = 0.0;
			sobelValY = 0.0;
			for (r2 = -1; r2 <= 1; r2++)
			{
				for (c2 = -1; c2 <= 1; c2++)
				{
					sobelValX += greyScale[(r + r2) * COLS + (c + c2)] * gx[(r2 + 1) * 3 + (c2 + 1)];
					sobelValY += greyScale[(r + r2) * COLS + (c + c2)] * gy[(r2 + 1) * 3 + (c2 + 1)];
				}
			}
			sobel[r * COLS + c] = sqrt(pow(sobelValX, 2) + pow(sobelValY, 2));
		}
	}

	//Normalize the sobel filtered image
	max = sobel[0];
	min = sobel[0];
	for (i = 0; i < (ROWS * COLS); i++) //find the max and min
	{
		if (min > sobel[i])
			min = sobel[i];
		if (max < sobel[i])
			max = sobel[i];
	}
	for (i = 0; i < (ROWS * COLS); i++) //normalize msfImage
	{
		sobelImage[i] = (255) * ((sobel[i] - min) / (max - min));
	}
}

void PaintSobel()

{
	PAINTSTRUCT			Painter;
	HDC					hDC;
	BITMAPINFOHEADER	bm_info_header;
	BITMAPINFO* bm_info;
	int					i, r, c, DISPLAY_ROWS, DISPLAY_COLS;
	unsigned char* sobel;

	if (OriginalImage == NULL)
		return;		/* no image to draw */

			  /* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
	DISPLAY_ROWS = ROWS;
	DISPLAY_COLS = COLS;
	if (DISPLAY_ROWS % 4 != 0)
		DISPLAY_ROWS = (DISPLAY_ROWS / 4 + 1) * 4;
	if (DISPLAY_COLS % 4 != 0)
		DISPLAY_COLS = (DISPLAY_COLS / 4 + 1) * 4;
	sobel = (unsigned char*)calloc(DISPLAY_ROWS * DISPLAY_COLS, 1);
	for (r = 0; r < ROWS; r++)
		for (c = 0; c < COLS; c++)
			sobel[r * DISPLAY_COLS + c] = sobelImage[r * COLS + c];

	BeginPaint(MainWnd, &Painter);
	hDC = GetDC(MainWnd);
	bm_info_header.biSize = sizeof(BITMAPINFOHEADER);
	bm_info_header.biWidth = DISPLAY_COLS;
	bm_info_header.biHeight = -DISPLAY_ROWS;
	bm_info_header.biPlanes = 1;
	bm_info_header.biBitCount = 8;
	bm_info_header.biCompression = BI_RGB;
	bm_info_header.biSizeImage = 0;
	bm_info_header.biXPelsPerMeter = 0;
	bm_info_header.biYPelsPerMeter = 0;
	bm_info_header.biClrUsed = 256;
	bm_info_header.biClrImportant = 256;
	bm_info = (BITMAPINFO*)calloc(1, sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
	bm_info->bmiHeader = bm_info_header;
	for (i = 0; i < 256; i++)
	{
		bm_info->bmiColors[i].rgbBlue = bm_info->bmiColors[i].rgbGreen = bm_info->bmiColors[i].rgbRed = i;
		bm_info->bmiColors[i].rgbReserved = 0;
	}

	SetDIBitsToDevice(hDC, 0, 0, DISPLAY_COLS, DISPLAY_ROWS, 0, 0,
		0, /* first scan line */
		DISPLAY_ROWS, /* number of scan lines */
		sobel, bm_info, DIB_RGB_COLORS);
	ReleaseDC(MainWnd, hDC);
	EndPaint(MainWnd, &Painter);

	free(sobel);
	free(bm_info);
}


void RubberBandThread()
{
	HDC					hDC;

	int iterations = 200;
	int window = 7;
	double internalE1[WINDOWSZ], internalE2[WINDOWSZ], externalE[WINDOWSZ];
	double norminternalE1[WINDOWSZ], norminternalE2[WINDOWSZ], normexternalE[WINDOWSZ];
	double sumDistance, avgDistance, totalEnergy[WINDOWSZ], max, min;
	int j, E1count, E2count, E3count, ii, jj, minIndex, iii, i, totalData, r, c, x, y;
	char text[320];

	totalData = rbIndex;
	int* xPoints, * yPoints;
	int E1weight = 2;
	int E2weight = 1.2;
	int E3weight = 2;
	//int xPoints[1000], yPoints[1000];

	xPoints = (int*)calloc(totalData, sizeof(int));
	yPoints = (int*)calloc(totalData, sizeof(int));
	for (ii = 0; ii < rbIndex; ii++)
	{
		xPoints[ii] = rubberbandX[ii];
		yPoints[ii] = rubberbandY[ii];
	}

	for (ii = 0; ii < iterations; ii++) //run for 30 total iterations
	{
		sumDistance = 0;
		for (j = 0; j < totalData; j++)
		{
			//printf("here %d\n",j);
			//Calculate the first internal energy
			E1count = 0;
			for (r = (-window / 2); r <= window / 2; r++)
			{
				for (c = (-window / 2); c <= window / 2; c++)
				{
					if (j == totalData - 1)
						internalE1[E1count] = pow(r + yPoints[j] - yPoints[0], 2) + pow(c + xPoints[j] - xPoints[0], 2);
					else
						internalE1[E1count] = pow(r + yPoints[j] - yPoints[j + 1], 2) + pow(c + xPoints[j] - xPoints[j + 1], 2);
					E1count++;
				}
			}

			//Calculate the second internal energy
			E2count = 0;
			sumDistance = 0.0;
			for (iii = 0; iii < totalData; iii++)
			{
				if (iii == totalData - 1)
				{
					sumDistance += sqrt(pow(yPoints[iii] - yPoints[0], 2) + pow(xPoints[iii] - xPoints[0], 2));
				}

				else
				{
					sumDistance += sqrt(pow(yPoints[iii] - yPoints[iii + 1], 2) + pow(xPoints[iii] - xPoints[iii + 1], 2));
				}

			}
			avgDistance = sumDistance / totalData;
			//printf("\n%f\n",avgDistance);
			for (r = (-window / 2); r <= window / 2; r++)
			{
				for (c = (-window / 2); c <= window / 2; c++)
				{
					if (j == totalData - 1)
						internalE2[E2count] = pow(avgDistance - sqrt(pow(r + yPoints[j] - yPoints[0], 2) + pow(c + xPoints[j] - xPoints[0], 2)), 2);
					else
						internalE2[E2count] = pow(avgDistance - sqrt(pow(r + yPoints[j] - yPoints[j + 1], 2) + pow(c + xPoints[j] - xPoints[j + 1], 2)), 2);
					E2count++;
				}
			}

			//Calculate the external energy
			E3count = 0;
			for (r = (-window / 2); r <= window / 2; r++)
			{
				for (c = (-window / 2); c <= window / 2; c++)
				{
					externalE[E3count] = ((float)sobelImage[(r + yPoints[j]) * COLS + (c + xPoints[j])] / 255);
					externalE[E3count] = sqrt(pow((externalE[E3count] - 1.0), 2));
					E3count++;
				}
			}
			//Normalize internal energies
			max = internalE1[0];
			min = internalE1[0];
			for (i = 0; i < (window * window); i++) //find the max and min
			{
				if (min > internalE1[i])
					min = internalE1[i];
				if (max < internalE1[i])
					max = internalE1[i];
			}
			for (i = 0; i < (window * window); i++)
			{
				norminternalE1[i] = (1) * ((internalE1[i] - min) / (max - min));
			}
			max = internalE2[0];
			min = internalE2[0];
			for (i = 0; i < (window * window); i++) //find the max and min
			{
				if (min > internalE2[i])
					min = internalE2[i];
				if (max < internalE2[i])
					max = internalE2[i];
			}
			for (i = 0; i < (window * window); i++)
			{
				norminternalE2[i] = (1) * ((internalE2[i] - min) / (max - min));
			}

			//Add up all the final energies
			for (jj = 0; jj < window * window; jj++)
			{
				totalEnergy[jj] = E1weight * norminternalE1[jj] + E2weight * norminternalE2[jj] + E3weight * externalE[jj];
			}
			//find the min energy location
			min = totalEnergy[0];
			minIndex = 0;
			for (jj = 0; jj < window * window; jj++)
			{
				if (min > totalEnergy[jj])
				{
					min = totalEnergy[jj];
					minIndex = jj;
				}
			}
			//Update the contour point location (least energy)
			xPoints[j] = xPoints[j] + (minIndex % window) - (window / 2);
			yPoints[j] = yPoints[j] + (minIndex / window) - (window / 2);
		}//totalData loop
		//Need to repaint the image and draw on the new contour at each iteration
		Sleep(20);
		PaintImage();

		sprintf(text, "Iteration: %d", ii + 1);

		for (i = 0; i < totalData;i++)
		{
			hDC = GetDC(MainWnd);
			TextOut(hDC, 0, 0, text, strlen(text));
			TextOut(hDC, 0, 0, text, strlen(text));
			for (x = -2; x <= 2; x++)
				for (y = -2; y <= 2; y++)
					SetPixel(hDC, xPoints[i] + x, yPoints[i] + y, RGB(0, 0, 255));
			ReleaseDC(MainWnd, hDC);
		}

	}//Iteration loop
	//Update the the ruberband X,Y arrays with the final points
	for (i = 0; i < totalData; i++)
	{
		rubberbandX[i] = xPoints[i];
		rubberbandY[i] = yPoints[i];
	}
}

void BalloonThread()
{
	HDC					hDC;

	int iterations;
	int window = 7;
	double internalE1[WINDOWSZ], internalE2[WINDOWSZ], internalE3[WINDOWSZ], internalE4[WINDOWSZ], externalE[WINDOWSZ];
	double norminternalE1[WINDOWSZ], norminternalE2[WINDOWSZ], norminternalE3[WINDOWSZ], norminternalE4[WINDOWSZ], normexternalE[WINDOWSZ];
	double sumDistance, avgDistance, totalEnergy[WINDOWSZ], max, min;
	int j, E1count, E2count, E3count, E4count, ii, jj, minIndex, iii, i, totalData, r, c, x, y;
	char text[320];

	int* xPoints, * yPoints;
	int E1weight, E2weight, E3weight, E4weight, E5weight;
	int hushCheck;
	if (strcmp("C:\\Users\\timdriscoll\\OneDrive - Clemson University\\Documents\\Clemson Masters\\Fall 2021\\ECE 6310\\Final Project\\hushpuppies-biscuits.pnm", filename) == 0)
		hushCheck = 1;
	else if(strcmp("C:\\Users\\timdriscoll\\OneDrive - Clemson University\\Documents\\Clemson Masters\\Fall 2021\\ECE 6310\\Final Project\\macaroni-kale.pnm", filename) == 0)
		hushCheck = 1;
	else
		hushCheck = 0;
	if (hushCheck)
	{
		E1weight = -1.0; //Keeping points evenly spaced
		E2weight = 0.9; //restrict outward movement ****** 1
		E3weight = -1.2; //move away from average point of contour ****** 2 (energy driving the contour to move away from center)
		E4weight = 3.0; //External image (gradient descent)
		E5weight = 1.0;
		iterations = 80;
	}
	else
	{
		E1weight = -1.1; //Keeping points evenly spaced
		E2weight = 1; //restrict outward movement ****** 1
		E3weight = -1.5; //move away from average point of contour ****** 2
		E4weight = 3.5; //External image (gradient descent)
		E5weight = 0;
		iterations = 100;
	}
	

	totalData = balloonIndex / 3;

	xPoints = (int*)calloc(totalData, sizeof(int));
	yPoints = (int*)calloc(totalData, sizeof(int));
	i = 0;
	for (ii = 0; ii < balloonIndex; ii++)
	{
		if (ii % 3 == 0) //Downsample to every third point
		{
			xPoints[i] = balloonX[ii];
			yPoints[i] = balloonY[ii];
			i++;
		}
	}


	for (ii = 0; ii < iterations; ii++) 
	{
		if (ii > 60 && hushCheck)
		{
			E3weight = -1.0;
			E4weight = 6.0;
		}
			
		sumDistance = 0;
		for (j = 0; j < totalData; j++)
		{
			//Calculate the first internal energy
			E1count = 0;
			for (r = (-window / 2); r <= window / 2; r++)
			{
				for (c = (-window / 2); c <= window / 2; c++)
				{
					if (j == totalData - 1)
						internalE1[E1count] = pow(r + yPoints[j-1] - yPoints[0], 2) + pow(c + xPoints[j-1] - xPoints[0], 2);
					else if(j == 0)
						internalE1[E1count] = pow(r + yPoints[totalData-1] - yPoints[j + 1], 2) + pow(c + xPoints[totalData-1] - xPoints[j + 1], 2);
					else
						internalE1[E1count] = pow(r + yPoints[j - 1] - yPoints[j + 1], 2) + pow(c + xPoints[j - 1] - xPoints[j + 1], 2);
					E1count++;
				}
			}

			//Calculate the second internal energy
			E2count = 0;
			sumDistance = 0.0;
			int sumX = 0.0;
			int avgX = 0.0;
			int sumY = 0.0;
			int avgY = 0.0;
			for (iii = 0; iii < totalData; iii++)
			{
				if (iii == totalData - 1)
				{
					sumDistance += sqrt(pow(yPoints[iii] - yPoints[0], 2) + pow(xPoints[iii] - xPoints[0], 2));
				}
				else
				{
					sumDistance += sqrt(pow(yPoints[iii] - yPoints[iii + 1], 2) + pow(xPoints[iii] - xPoints[iii + 1], 2));
				}
				sumX += xPoints[iii];
				sumY += yPoints[iii];
			}
			avgDistance = sumDistance / totalData;
			avgX = sumX / totalData;
			avgY = sumY / totalData;
			for (r = (-window / 2); r <= window / 2; r++)
			{
				for (c = (-window / 2); c <= window / 2; c++)
				{
					if (r == 0 && c == 0)
						internalE2[E2count] = 0;
					else
						internalE2[E2count] = pow(r, 2) + pow(c, 2);
					internalE3[E2count] = pow(abs(xPoints[j] + c - avgX), 2) + pow(abs(yPoints[j] + r - avgY), 2);
					E2count++;
				}
			}

			//Calculate the 4th internal energy
			E4count = 0;
			sumDistance = 0.0;
			for (iii = 0; iii < totalData; iii++)
			{
				if (iii == totalData - 1)
				{
					sumDistance += sqrt(pow(yPoints[iii] - yPoints[0], 2) + pow(xPoints[iii] - xPoints[0], 2));
				}

				else
				{
					sumDistance += sqrt(pow(yPoints[iii] - yPoints[iii + 1], 2) + pow(xPoints[iii] - xPoints[iii + 1], 2));
				}

			}
			avgDistance = sumDistance / totalData;
			//printf("\n%f\n",avgDistance);
			for (r = (-window / 2); r <= window / 2; r++)
			{
				for (c = (-window / 2); c <= window / 2; c++)
				{
					if (j == totalData - 1)
						internalE4[E4count] = pow(avgDistance - sqrt(pow(r + yPoints[j] - yPoints[0], 2) + pow(c + xPoints[j] - xPoints[0], 2)), 2);
					else
						internalE4[E4count] = pow(avgDistance - sqrt(pow(r + yPoints[j] - yPoints[j + 1], 2) + pow(c + xPoints[j] - xPoints[j + 1], 2)), 2);
					E4count++;
				}
			}

			//Calculate the external energy
			E3count = 0;
			if (hushCheck)
			{
				for (r = (-window / 2); r <= window / 2; r++)
				{
					for (c = (-window / 2); c <= window / 2; c++)
					{
						if ((r + yPoints[j]) > ROWS && (c + xPoints[j]) > COLS)
							externalE[E3count] = ((float)sobelImage[ROWS * COLS] / 255);
						else if ((r + yPoints[j]) > ROWS)
							externalE[E3count] = ((float)sobelImage[(ROWS)*COLS + (c + xPoints[j])] / 255);
						else if ((c + xPoints[j]) > COLS)
							externalE[E3count] = ((float)sobelImage[(r + yPoints[j]) * COLS + (COLS)] / 255);
						else
							externalE[E3count] = ((float)sobelImage[(r + yPoints[j]) * COLS + (c + xPoints[j])] / 255);
						externalE[E3count] = sqrt(pow((externalE[E3count] - 1.0), 2));
						E3count++;
					}
				}
			}
			else
			{
				for (r = (-window / 2); r <= window / 2; r++)
				{
					for (c = (-window / 2); c <= window / 2; c++)
					{
						externalE[E3count] = ((float)sobelImage[(r + yPoints[j]) * COLS + (c + xPoints[j])] / 255);
						externalE[E3count] = sqrt(pow((externalE[E3count] - 1.0), 2));
						E3count++;
					}
				}
			}
			
			//Normalize internal energies
			max = internalE1[0];
			min = internalE1[0];
			for (i = 0; i < (window * window); i++) //find the max and min
			{
				if (min > internalE1[i])
					min = internalE1[i];
				if (max < internalE1[i])
					max = internalE1[i];
			}
			for (i = 0; i < (window * window); i++)
			{
				norminternalE1[i] = (1) * ((internalE1[i] - min) / (max - min));
			}
			max = internalE2[0];
			min = internalE2[0];
			for (i = 0; i < (window * window); i++) //find the max and min
			{
				if (min > internalE2[i])
					min = internalE2[i];
				if (max < internalE2[i])
					max = internalE2[i];
			}
			for (i = 0; i < (window * window); i++)
			{
				norminternalE2[i] = (1) * ((internalE2[i] - min) / (max - min));
			}
			max = internalE3[0];
			min = internalE3[0];
			for (i = 0; i < (window * window); i++) //find the max and min
			{
				if (min > internalE3[i])
					min = internalE3[i];
				if (max < internalE3[i])
					max = internalE3[i];
			}
			for (i = 0; i < (window * window); i++)
			{
				norminternalE3[i] = (1) * ((internalE3[i] - min) / (max - min));
			}
			max = internalE4[0];
			min = internalE4[0];
			for (i = 0; i < (window * window); i++) //find the max and min
			{
				if (min > internalE4[i])
					min = internalE4[i];
				if (max < internalE4[i])
					max = internalE4[i];
			}
			for (i = 0; i < (window * window); i++)
			{
				norminternalE4[i] = (1) * ((internalE4[i] - min) / (max - min));
			}

			//Add up all the final energies
			for (jj = 0; jj < window * window; jj++)
			{
				totalEnergy[jj] = E1weight * norminternalE1[jj] + E2weight * norminternalE2[jj] + E3weight * norminternalE3[jj] + E4weight * externalE[jj] + E5weight * norminternalE4[jj];
			}
			//find the min energy location
			min = totalEnergy[0];
			minIndex = 0;
			for (jj = 0; jj < window * window; jj++)
			{
				if (min > totalEnergy[jj])
				{
					min = totalEnergy[jj];
					minIndex = jj;
				}
			}
			//Update the contour point location (least energy)
			xPoints[j] = xPoints[j] + (minIndex % window) - (window / 2);
			yPoints[j] = yPoints[j] + (minIndex / window) - (window / 2);
		}//totalData loop
		//Need to repaint the image and draw on the new contour at each iteration
		Sleep(20);
		PaintImage();

		sprintf(text, "Iteration: %d", ii + 1);

		for (i = 0; i < totalData;i++)
		{
			hDC = GetDC(MainWnd);
			TextOut(hDC, 0, 0, text, strlen(text));
			for (x = -2; x <= 2; x++)
				for (y = -2; y <= 2; y++)
					SetPixel(hDC, xPoints[i] + x, yPoints[i] + y, RGB(0, 0, 255));
			ReleaseDC(MainWnd, hDC);
		}

	}//Iteration loop
	//Update the the balloon X,Y arrays with the final points
	for (i = 0; i < totalData; i++)
	{
		balloonX[i] = xPoints[i];
		balloonY[i] = yPoints[i];
	}
}

void NeutralThread()
{
	HDC					hDC;

	int iterations = 25;
	int window = 7;
	double internalE1[WINDOWSZ], internalE2[WINDOWSZ], externalE[WINDOWSZ];
	double norminternalE1[WINDOWSZ], norminternalE2[WINDOWSZ], normexternalE[WINDOWSZ];
	double sumDistance, avgDistance, totalEnergy[WINDOWSZ], max, min;
	int j, E1count, E2count, E3count, ii, jj, minIndex, iii, i, totalData, r, c, x, y;
	char text[320];

	totalData = neutralIndex;
	int* xPoints, * yPoints;
	int E1weight = 1;
	int E2weight = 6;
	int E3weight = 3;
	//int xPoints[1000], yPoints[1000];

	xPoints = (int*)calloc(totalData, sizeof(int));
	yPoints = (int*)calloc(totalData, sizeof(int));
	for (ii = 0; ii < neutralIndex; ii++)
	{
		xPoints[ii] = neutralX[ii];
		yPoints[ii] = neutralY[ii];
	}

	for (ii = 0; ii < iterations; ii++) 
	{
		sumDistance = 0;
		for (j = 0; j < totalData; j++)
		{
			if (j == match)
				continue;

			E1count = 0;
			for (r = (-window / 2); r <= window / 2; r++)
			{
				for (c = (-window / 2); c <= window / 2; c++)
				{
					if (j == totalData - 1)
						internalE1[E1count] = pow(r + yPoints[j] - yPoints[0], 2) + pow(c + xPoints[j] - xPoints[0], 2);
					else
						internalE1[E1count] = pow(r + yPoints[j] - yPoints[j + 1], 2) + pow(c + xPoints[j] - xPoints[j + 1], 2);
					E1count++;
				}
			}

			//Calculate the second internal energy
			E2count = 0;
			sumDistance = 0.0;
			for (iii = 0; iii < totalData; iii++)
			{
				if (iii == totalData - 1)
				{
					sumDistance += sqrt(pow(yPoints[iii] - yPoints[0], 2) + pow(xPoints[iii] - xPoints[0], 2));
				}

				else
				{
					sumDistance += sqrt(pow(yPoints[iii] - yPoints[iii + 1], 2) + pow(xPoints[iii] - xPoints[iii + 1], 2));
				}

			}
			avgDistance = sumDistance / totalData;
			//printf("\n%f\n",avgDistance);
			for (r = (-window / 2); r <= window / 2; r++)
			{
				for (c = (-window / 2); c <= window / 2; c++)
				{
					if (j == totalData - 1)
						internalE2[E2count] = pow(avgDistance - sqrt(pow(r + yPoints[j] - yPoints[0], 2) + pow(c + xPoints[j] - xPoints[0], 2)), 2);
					else
						internalE2[E2count] = pow(avgDistance - sqrt(pow(r + yPoints[j] - yPoints[j + 1], 2) + pow(c + xPoints[j] - xPoints[j + 1], 2)), 2);
					E2count++;
				}
			}

			//Calculate the external energy
			E3count = 0;
			for (r = (-window / 2); r <= window / 2; r++)
			{
				for (c = (-window / 2); c <= window / 2; c++)
				{
					externalE[E3count] = ((float)sobelImage[(r + yPoints[j]) * COLS + (c + xPoints[j])] / 255);
					externalE[E3count] = sqrt(pow((externalE[E3count] - 1.0), 2));
					E3count++;
				}
			}
			//Normalize internal energies
			max = internalE1[0];
			min = internalE1[0];
			for (i = 0; i < (window * window); i++) //find the max and min
			{
				if (min > internalE1[i])
					min = internalE1[i];
				if (max < internalE1[i])
					max = internalE1[i];
			}
			for (i = 0; i < (window * window); i++)
			{
				norminternalE1[i] = (1) * ((internalE1[i] - min) / (max - min));
			}
			max = internalE2[0];
			min = internalE2[0];
			for (i = 0; i < (window * window); i++) //find the max and min
			{
				if (min > internalE2[i])
					min = internalE2[i];
				if (max < internalE2[i])
					max = internalE2[i];
			}
			for (i = 0; i < (window * window); i++)
			{
				norminternalE2[i] = (1) * ((internalE2[i] - min) / (max - min));
			}

			//Add up all the final energies
			for (jj = 0; jj < window * window; jj++)
			{
				totalEnergy[jj] = E1weight * norminternalE1[jj] + E2weight * norminternalE2[jj] + E3weight * externalE[jj];
			}
			//find the min energy location
			min = totalEnergy[0];
			minIndex = 0;
			for (jj = 0; jj < window * window; jj++)
			{
				if (min > totalEnergy[jj])
				{
					min = totalEnergy[jj];
					minIndex = jj;
				}
			}
			//Update the contour point location (least energy)
			//if (j != match) //unless looking at the fixed point
			//{
				xPoints[j] = xPoints[j] + (minIndex % window) - (window / 2);
				yPoints[j] = yPoints[j] + (minIndex / window) - (window / 2);
			//}
			
		}//totalData loop
		//Need to repaint the image and draw on the new contour at each iteration
		Sleep(20);
		PaintImage();

		sprintf(text, "Iteration: %d", ii + 1);

		for (i = 0; i < totalData;i++)
		{
			hDC = GetDC(MainWnd);
			TextOut(hDC, 0, 0, text, strlen(text));
			TextOut(hDC, 0, 0, text, strlen(text));
			for (x = -2; x <= 2; x++)
				for (y = -2; y <= 2; y++)
					if(i == match)
						SetPixel(hDC, xPoints[i] + x, yPoints[i] + y, RGB(0, 255, 255));
					else
						SetPixel(hDC, xPoints[i] + x, yPoints[i] + y, RGB(0, 0, 255));
			ReleaseDC(MainWnd, hDC);
		}

	}//Iteration loop
	//Update the the ruberband X,Y arrays with the final points
	for (i = 0; i < totalData; i++)
	{
		neutralX[i] = xPoints[i];
		neutralY[i] = yPoints[i];
	}
	pointMove = 0;
}




