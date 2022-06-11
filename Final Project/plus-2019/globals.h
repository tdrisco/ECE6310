
#define SQR(x) ((x)*(x))	/* macro for square */
#ifndef M_PI			/* in case M_PI not found in math.h */
#define M_PI 3.1415927
#endif
#ifndef M_E
#define M_E 2.718282
#endif

#define MAX_FILENAME_CHARS	320

char	filename[MAX_FILENAME_CHARS];

HWND	MainWnd;

		// Display flags
int		ShowPixelCoords;

		// Image data
unsigned char	*OriginalImage, *greyScale, *sobelImage;
int				ROWS,COLS;

#define TIMER_SECOND	1			/* ID of timer used for animation */

#define WINDOWSZ		49

		// Drawing flags
int		TimerRow,TimerCol;
int		ThreadRow,ThreadCol;
int		ThreadRunning;
int		Sobel_switch, Original_switch, RubberBand_switch, Balloon_switch, Neutral_switch;
int		BigDots, match, shiftKey,pointMove;
int		mouse_x,mouse_y;
int		rubberbandX[100000], rubberbandY[100000], rubberbandCount, rbIndex;
int		balloonX[100000], balloonY[100000], balloonIndex;
int		neutralX[100000], neutralY[100000], neutralIndex;


		// Function prototypes
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
void PaintImage(); //Paint the original image loaded into the gui
void PaintSobel(); //Paint the sobel image 
void CalculateSobelImage(); //Calculate the sobel image from the original image
void CalculateGreyScale(); //Convert the RGB image into greyscale
void RubberBandThread(); //Function to perform rubber band method active contour
void BalloonThread(); //Function to perform balloon method active contouring
void NeutralThread(); //Neutral contouring model to actively contour after a point is manually moved by the user



