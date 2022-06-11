
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

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, "ID_PLUS_ICON");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = "ID_MAIN_MENU";
	wc.lpszClassName = "PLUS";



	if (!RegisterClass(&wc))
		return(FALSE);

	hWnd = CreateWindow("PLUS", "plus program",
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
		CW_USEDEFAULT, 0, 400, 400, NULL, NULL, hInstance, NULL);
	if (!hWnd)
		return(FALSE);

	ShowScrollBar(hWnd, SB_BOTH, FALSE);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	MainWnd = hWnd;

	ShowPixelCoords = 0;
	BigDots = 0;

	strcpy(filename, "");
	OriginalImage = NULL;
	ROWS = COLS = 0;

	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return(msg.wParam);
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam)

{
	HMENU				hMenu;
	OPENFILENAME		ofn;
	FILE* fpt;
	HDC					hDC;
	char				header[320], text[320];
	int					BYTES, xPos, yPos, x, y;
	int rValue, gValue, bValue;

	CHOOSECOLOR cc;                 // common dialog box structure 
	static COLORREF acrCustClr[16]; // array of custom colors 


	// Initialize CHOOSECOLOR 
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.lpCustColors = (LPDWORD)acrCustClr;
	cc.rgbResult = rgbCurrent;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;




	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_SHOWPIXELCOORDS:
			ShowPixelCoords = (ShowPixelCoords + 1) % 2;
			PaintImage();
			break;
		case ID_DISPLAY_BIGDOTS:
			BigDots = (BigDots + 1) % 2;
			PaintImage();
			break;
		case ID_FILE_LOAD:
			if (OriginalImage != NULL)
			{
				free(OriginalImage);
				OriginalImage = NULL;
			}
			memset(&(ofn), 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFile = filename;
			filename[0] = 0;
			ofn.nMaxFile = MAX_FILENAME_CHARS;
			ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
			ofn.lpstrFilter = "PPM files\0*.ppm\0All files\0*.*\0\0";
			if (!(GetOpenFileName(&ofn)) || filename[0] == '\0')
				break;		/* user cancelled load */
			if ((fpt = fopen(filename, "rb")) == NULL)
			{
				MessageBox(NULL, "Unable to open file", filename, MB_OK | MB_APPLMODAL);
				break;
			}
			fscanf(fpt, "%s %d %d %d", header, &COLS, &ROWS, &BYTES);
			if (strcmp(header, "P5") != 0 || BYTES != 255)
			{
				MessageBox(NULL, "Not a PPM (P5 greyscale) image", filename, MB_OK | MB_APPLMODAL);
				fclose(fpt);
				break;
			}
			OriginalImage = (unsigned char*)calloc(ROWS * COLS, 1);
			header[0] = fgetc(fpt);	/* whitespace character after header */
			fread(OriginalImage, 1, ROWS * COLS, fpt);
			fclose(fpt);
			SetWindowText(hWnd, filename);
			PaintImage();
			break;

		case ID_FILE_QUIT:
			ThreadRunning = FALSE;
			DestroyWindow(hWnd);
			break;

		case ID_REGIONGROWING_COLOR:
			hWnd = CreateWindow("Color", "Color program",
				WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
				CW_USEDEFAULT, 0, 400, 400, NULL, NULL, NULL, NULL);
			cc.hwndOwner = hWnd;
			if (ChooseColor(&cc) == TRUE)
			{
				rgbCurrent = cc.rgbResult;
				rValue = (int)GetRValue(rgbCurrent);
				gValue = rValue;
			}
			break;
		case ID_REGIONGROWING_CLEARREGIONGROW:
			playMode = FALSE;
			stepMode = FALSE;
			ThreadRunning = FALSE;
			PaintImage();
			break;
		case ID_REGIONGROWING_REGIONJOINPREDICATES:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_PredDG), hWnd, PredicateDBcallback);
			break;

		case ID_REGIONGROWING_PLAYMODE:
			playMode = !playMode;
			if (playMode)
			{
				stepMode = FALSE;
				stepTrigger = TRUE;
			}
			break;
		case ID_REGIONGROWING_STEPMODE:
			stepMode = !stepMode;
			if (stepMode)
			{
				playMode = FALSE;
			}
			break;
			break;
		}
		break;
	case WM_SIZE:		  /* could be used to detect when window size changes */
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_PAINT:
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_LBUTTONDOWN:case WM_RBUTTONDOWN:
		if (playMode || stepMode)
		{
			mouse_x = LOWORD(lParam);
			mouse_y = HIWORD(lParam);
			ThreadRunning = TRUE;
			_beginthread(regionGrowThread, 0, MainWnd);	/* start up a child thread to do other work while this thread continues GUI */
		}
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_MOUSEMOVE:
		if (ShowPixelCoords == 1)
		{
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			if (xPos >= 0 && xPos < COLS && yPos >= 0 && yPos < ROWS)
			{
				sprintf(text, "%d,%d=>%d     ", xPos, yPos, OriginalImage[yPos * COLS + xPos]);
				hDC = GetDC(MainWnd);
				TextOut(hDC, 0, 0, text, strlen(text));		/* draw text on the window */
				if (BigDots == 0)
					SetPixel(hDC, xPos, yPos, RGB(255, 0, 0));	/* color the cursor position red */
				else
				{
					for (x = -2; x <= 2; x++)
						for (y = -2; y <= 2; y++)
							SetPixel(hDC, xPos + x, yPos + y, RGB(255, 0, 0));
				}
				ReleaseDC(MainWnd, hDC);
			}
		}
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_KEYDOWN:
		if (wParam == 's' || wParam == 'S')
			PostMessage(MainWnd, WM_COMMAND, ID_SHOWPIXELCOORDS, 0);	  /* send message to self */
		if (wParam == 'j' || wParam == 'J')
		{
			stepTrigger = TRUE;
		}
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_TIMER:	  /* this event gets triggered every time the timer goes off */
		hDC = GetDC(MainWnd);
		SetPixel(hDC, TimerCol, TimerRow, RGB(0, 0, 255));	/* color the animation pixel blue */
		ReleaseDC(MainWnd, hDC);
		TimerRow++;
		TimerCol += 2;
		break;
	case WM_HSCROLL:	  /* this event could be used to change what part of the image to draw */
		PaintImage();	  /* direct PaintImage calls eliminate flicker; the alternative is InvalidateRect(hWnd,NULL,TRUE); UpdateWindow(hWnd); */
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_VSCROLL:	  /* this event could be used to change what part of the image to draw */
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	}

	hMenu = GetMenu(MainWnd);
	if (ShowPixelCoords == 1)
		CheckMenuItem(hMenu, ID_SHOWPIXELCOORDS, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_SHOWPIXELCOORDS, MF_UNCHECKED);
	if (BigDots == 1)
		CheckMenuItem(hMenu, ID_DISPLAY_BIGDOTS, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_DISPLAY_BIGDOTS, MF_UNCHECKED);
	if (playMode)
		CheckMenuItem(hMenu, ID_REGIONGROWING_PLAYMODE, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_REGIONGROWING_PLAYMODE, MF_UNCHECKED);
	if (stepMode)
		CheckMenuItem(hMenu, ID_REGIONGROWING_STEPMODE, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_REGIONGROWING_STEPMODE, MF_UNCHECKED);

	DrawMenuBar(hWnd);

	return(0L);
}




void PaintImage()

{
	PAINTSTRUCT			Painter;
	HDC					hDC;
	BITMAPINFOHEADER	bm_info_header;
	BITMAPINFO* bm_info;
	int					i, r, c, DISPLAY_ROWS, DISPLAY_COLS;
	unsigned char* DisplayImage;

	if (OriginalImage == NULL)
		return;		/* no image to draw */

			  /* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
	DISPLAY_ROWS = ROWS;
	DISPLAY_COLS = COLS;
	if (DISPLAY_ROWS % 4 != 0)
		DISPLAY_ROWS = (DISPLAY_ROWS / 4 + 1) * 4;
	if (DISPLAY_COLS % 4 != 0)
		DISPLAY_COLS = (DISPLAY_COLS / 4 + 1) * 4;
	DisplayImage = (unsigned char*)calloc(DISPLAY_ROWS * DISPLAY_COLS, 1);
	for (r = 0; r < ROWS; r++)
		for (c = 0; c < COLS; c++)
			DisplayImage[r * DISPLAY_COLS + c] = OriginalImage[r * COLS + c];

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
		DisplayImage, bm_info, DIB_RGB_COLORS);
	ReleaseDC(MainWnd, hDC);
	EndPaint(MainWnd, &Painter);

	free(DisplayImage);
	free(bm_info);
}




void regionGrowThread(HWND AnimationWindowHandle)

{
	HDC		hDC;
	char	text[300];
	unsigned char* labels;
	int* indices,r2,c2,RegionSize, i;
	double avg, var;

	labels = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
	/* used to quickly erase small grown regions */
	indices = (int*)calloc(ROWS * COLS, sizeof(int));

	avg = 0.0;
	var = 0.0;

	TotalRegions = 0;

	avg = var = 0.0;	/* compute average and variance in 7x7 window */
	for (r2 = -3; r2 <= 3; r2++)
		for (c2 = -3; c2 <= 3; c2++)
			avg += (double)(OriginalImage[(mouse_y + r2) * COLS + (mouse_x + c2)]);
	avg /= 49.0;
	for (r2 = -3; r2 <= 3; r2++)
		for (c2 = -3; c2 <= 3; c2++)
			var += SQR(avg - (double)OriginalImage[(mouse_y + r2) * COLS + (mouse_x + c2)]);
	var = sqrt(var) / 49.0;
	if (var < 5.0)	/* condition for seeding a new region is low var */
	{
		TotalRegions++;
		RegionGrow(OriginalImage, labels, ROWS, COLS, mouse_y, mouse_x, 0, TotalRegions, indices, &RegionSize);
		if (RegionSize < 100)
		{	/* erase region (relabel pixels back to 0) */
			for (i = 0; i < RegionSize; i++)
				labels[indices[i]] = 0;
			TotalRegions--;
		}
	}

}



void RegionGrow(unsigned char* image,	/* image data */
	unsigned char* labels,	/* segmentation labels */
	int ROWS, int COLS,	/* size of image */
	int r, int c,		/* pixel to paint from */
	int paint_over_label,	/* image label to paint over */
	int new_label,		/* image label for painting */
	int* indices,		/* output:  indices of pixels painted */
	int* count)		/* output:  count of pixels painted */
{
	HDC		hDC;
	int	r2, c2;
	int	queue[MAX_QUEUE], qh, qt;
	int	average, total;	/* average and total intensity in growing region */

	*count = 0;
	if (labels[r * COLS + c] != paint_over_label)
		return;
	labels[r * COLS + c] = new_label;
	average = total = (int)image[r * COLS + c];
	if (indices != NULL)
		indices[0] = r * COLS + c;
	queue[0] = r * COLS + c;
	qh = 1;	/* queue head */
	qt = 0;	/* queue tail */
	(*count) = 1;
	while (qt != qh && ThreadRunning)
	{
		if (stepTrigger)
		{
			if ((*count) % 50 == 0)	/* recalculate average after each 50 pixels join */
			{
				average = total / (*count);
			}
			for (r2 = -1; r2 <= 1; r2++)
			{
				for (c2 = -1; c2 <= 1; c2++)
				{
					if (r2 == 0 && c2 == 0)
						continue;
					if ((queue[qt] / COLS + r2) < 0 || (queue[qt] / COLS + r2) >= ROWS ||
						(queue[qt] % COLS + c2) < 0 || (queue[qt] % COLS + c2) >= COLS)
						continue;
					if (labels[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] != paint_over_label)
						continue;
					/* test criteria to join region */
					if (abs((int)(image[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2])
						- average) > pred1)
						continue;
					if ((sqrt(pow((r - (queue[qt] / COLS + r2)), 2) + pow((c - (queue[qt] % COLS + c2)), 2))) > pred2)
						continue;
					labels[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] = new_label;
					hDC = GetDC(MainWnd);
					SetPixel(hDC, queue[qt] % COLS + c2, (queue[qt] / COLS + r2), rgbCurrent);
					ReleaseDC(MainWnd, hDC);
					if (indices != NULL)
						indices[*count] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
					total += image[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2];
					(*count)++;
					queue[qh] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
					qh = (qh + 1) % MAX_QUEUE;
					if (qh == qt)
					{
						exit(0);
					}
				}
			}
			qt = (qt + 1) % MAX_QUEUE;
			if (playMode)
			{
				Sleep(1); // pause 1ms
			}
			else if (stepMode)
			{
				stepTrigger = FALSE;
			}
			else if (!playMode)
			{
				stepTrigger = FALSE;
			}
		}
	}
}


BOOL CALLBACK PredicateDBcallback(HWND hPredDlg, UINT uMsg, WPARAM wParam, LPARAM Lparam)
{

	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			pred1 = GetDlgItemInt(hPredDlg, IDC_Pred1, NULL, TRUE);

			pred2 = GetDlgItemInt(hPredDlg, IDC_Pred2, NULL, TRUE);

			EndDialog(hPredDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			if (MessageBoxW(hPredDlg, L"Are you sure you want to cancel?", L"Warning", MB_YESNO) == IDYES)
				EndDialog(hPredDlg, IDCANCEL);
			pred1 = 100;
			pred2 = 200;
			return TRUE;
		}
		break;
	case WM_CLOSE:
		EndDialog(hPredDlg, WM_CLOSE);
	default:
		return FALSE;
	}
	return FALSE;

}