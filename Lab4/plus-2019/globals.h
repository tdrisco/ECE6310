
#define SQR(x) ((x)*(x))	/* macro for square */
#ifndef M_PI			/* in case M_PI not found in math.h */
#define M_PI 3.1415927
#endif
#ifndef M_E
#define M_E 2.718282
#endif

#define MAX_FILENAME_CHARS	320
#define MAX_QUEUE 10000	/* max perimeter size (pixels) of border wavefront */

char	filename[MAX_FILENAME_CHARS];

HWND	MainWnd;

		// Display flags
int		ShowPixelCoords;

		// Image data
unsigned char	*OriginalImage;
int				ROWS,COLS;

#define TIMER_SECOND	1			/* ID of timer used for animation */

		// Drawing flags
int		TimerRow,TimerCol;
int		ThreadRow,ThreadCol;
int		ThreadRunning;
int		BigDots;
int		pred1 = 100;
int		pred2 = 200;
int		TotalRegions;
BOOL	playMode = FALSE;
BOOL	stepMode = FALSE;
BOOL	stepTrigger = FALSE;
BOOL	ThreadRunning;
int		mouse_x,mouse_y;
static DWORD rgbCurrent = RGB(255,0,0);

		// Function prototypes
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
BOOL CALLBACK PredicateDBcallback(HWND, UINT, WPARAM, LPARAM);
void PaintImage();
void regionGrowThread(void *);		/* passes address of window */
void RegionGrow(unsigned char*, unsigned char*, int, int, int, int, int, int, int*, int*);
