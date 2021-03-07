// We want to be explicit about the number of bytes we're allocating in our variables, so we're
// importing more explicit definitions
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <math.h>

// Defining these this way because it's easier to understand drawing to the screen
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

// This will allow you to make windows create a window for you
#include <windows.h>

#define BACKGROUND_COLOR 0x00000000

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TIME_BEGIN_PERIOD(name) MMRESULT name(UINT uPeriod)
typedef TIME_BEGIN_PERIOD(time_begin_period);
TIME_BEGIN_PERIOD(TimeBeginPeriodStub){
	return TIMERR_NOCANDO;
}
static time_begin_period *timeBeginPeriod_ = TimeBeginPeriodStub;
#define timeBeginPeriod timeBeginPeriod_

struct win32_offscreen_buffer {
    BITMAPINFO Info; // it's not really the whole BITMAPINFO we care about, just the header
    void *Memory; 	// this be the memory where the data of the DIB is stored;
	// the ^ variable is not the memory itself, but a pointer to the location of the memory
    int Width;
    int Height;
    int Pitch;
	int BytesPerPixel;
};

struct bmp_pixel {
	uint8 blue;
	uint8 green;
	uint8 red;
};

struct bmp_image_data {
	uint32 Width;
	uint32 Height;
	uint32 BytesPerPixel;
	uint32 Size;
	uint8 *ImageData;
};

//This function will be for opening BMP files
bmp_image_data GetBmpImageData(const char *Filename) {
	bmp_image_data Result = {};
	
	if(FILE *filePointer = fopen(Filename, "rb")) {		
		// get file size (in BMP it is at 0x0002 or two)
		uint32 FileSize;
		fseek(filePointer, 2, SEEK_SET);
		fread(&FileSize, sizeof(uint32), 1, filePointer);
		
		// get the data offset (in BMP It is at 0x000A)
		uint32 DataOffset;
		fseek(filePointer, 0x000A, SEEK_SET);
		fread(&DataOffset, sizeof(uint32), 1, filePointer);
		fseek(filePointer, DataOffset, SEEK_SET);

		// get width (0x0012 in BMP)  and height (0x0016 in BMP)
		uint32 Width;
		fseek(filePointer, 0x0012, SEEK_SET);
		fread(&Width, 4, 1, filePointer);
		uint32 Height;
		fseek(filePointer, 0x0016, SEEK_SET);
		fread(&Height, 4, 1, filePointer);

		// get the bytesperpixel (0x001C in BMP)
		uint32 BytesPerPixel;
		int16 BitsPerPixel;
		fseek(filePointer, 0x001C, SEEK_SET);
		fread(&BitsPerPixel, 2, 1, filePointer);
		BytesPerPixel = ((int32)BitsPerPixel) / 8;
		
		// If the BMP is some other format besides 24bit or 32bit, CHUCK IT!
		if(BytesPerPixel<3||BytesPerPixel>4){
			std::cout << "ERROR: This program only accepts 24bit or 32bit BMP files'" << Filename << "'.\n";
			return Result;
		}
		
		// Get the Width in bytes
		uint32 WidthBytes = Width*BytesPerPixel;
	
		// get the padded width
		uint32 PaddedWidth = Width + Width%4;
	
		// and the padded width in bytes
		uint32 PaddedWidthBytes = ceil((float)(WidthBytes/4.0f))*4;
		
		// get the actual size of the data
		uint32 ActualDataSize = PaddedWidthBytes*Height;

		// get the size of the Data we want
		uint32 DataSize = Width*Height*BytesPerPixel;
		
		// get the size of the final Data
		uint32 FinalDataSize = Width*Height*4;
		
		// create a pointer to point to some allocated
		// space for our image data
		uint8 *ImageData;
		
		// allocate space for the data we want
		ImageData = (uint8*)malloc(DataSize);
		memset(ImageData, 0xFFFFFFFF, DataSize);
		
		// Write the image data to the allocated space
		// Grab the last row of the memory we allocated
		uint8 *CurrentRow = ImageData+((Height-1)*WidthBytes);
		for (int i = 0; i < Height; i++)
		{
			fseek(filePointer, DataOffset+(i*PaddedWidthBytes), SEEK_SET);
			fread(CurrentRow, 1, WidthBytes, filePointer);
			CurrentRow -= WidthBytes;
		}
		
		uint8 *TestThePixels = ImageData;
		uint32 TestPixel;
		for(int i=0;i<DataSize;i++) {
			TestPixel = *TestThePixels<<8;
			TestPixel = TestPixel>>8;
			*TestThePixels++;
		}
		
		//If it's a 24bit BMP, convert it to a 32Bit BMP
		if(BytesPerPixel==3) {
			// Divide the size by the amount of bytes in a 24bit pixel (3),
			// then multiply that divided number by the number of
			// bytes in a 32bit pixel (4)
			uint32 NewDataSize = (DataSize/3)*4;
			
			// Allocate some space to shove the converted data into 
			uint8 *ImageData32Bit = (uint8*)malloc(NewDataSize);
			
			uint32 NumberOfPixels = Width*Height;
			
			// Convert the data into 32bit, then shove the data into
			// our memory one pixel at a time
			bmp_pixel *OldDataPointer = (bmp_pixel*)ImageData;
			uint32 *NewDataPointer = (uint32*)ImageData32Bit;
			uint32 DIBPixel;
			for (int i = 0; i < NumberOfPixels; i++) {
				DIBPixel = (0xFF << 24) |(OldDataPointer->red << 16) | (OldDataPointer->green<<8) | (OldDataPointer->blue); 
				*NewDataPointer = DIBPixel;
				*NewDataPointer++;
				*OldDataPointer++;
			}
			// Change the BytesPerPixel to 4
			BytesPerPixel=4;
			// Remove the allocated memory we created for the original bit image
			free(ImageData);
			
			// Point it's pointer to the new memory with the 32 bit data
			ImageData = ImageData32Bit;
			DataSize=NewDataSize;
			
		}
		
		fclose(filePointer);
		Result.Width = Width;
		Result.Height = Height;
		Result.BytesPerPixel = BytesPerPixel;
		Result.Size = DataSize;
		Result.ImageData = ImageData;
	} else {
		std::cout << "ERROR: Unable to open BMP image file '" << Filename << "'.\n";
	}
	return Result;
}

void ClearBmpImageData(bmp_image_data BmpToClear) {
	free(BmpToClear.ImageData);
}

bmp_image_data GetGenericImageData (const char *Filename) {
	bmp_image_data Result = {};
	int Width,Height,BytesPerPixel;
	unsigned char *data = stbi_load("images\\01.png", &Width, &Height, &BytesPerPixel, 0);
	Result.Width = Width;
	Result.Height = Height;
	Result.BytesPerPixel=BytesPerPixel;
	Result.Size = Width*Height*BytesPerPixel;
	Result.ImageData = data;
	return Result;
}


void DrawImage (bmp_image_data ImageData, int ImageX, int ImageY, win32_offscreen_buffer *Buffer) {
			//std::cout << "ImageH:" << ImageY + (int)ImageData.Height << "\n";
		if((Buffer->Height >= ImageY&&ImageY + (int)ImageData.Height >=0)
			&&(Buffer->Width >= ImageX&&ImageX + (int)ImageData.Width >=0)
			){
			int ImageStartX = ImageX;
			int ImageStartY = ImageY;
			
			// Calculate Y
			if(ImageStartY<0) {
				ImageStartY = 0;
			}
			//std::cout << "ImageStartY:" << ImageStartY << "\n";
			int ImageEndY = (ImageY + (int)ImageData.Height);
			if(ImageEndY > Buffer->Height-0){
				ImageEndY = Buffer->Height-0;
			}
			
			
			// Calculate X 
			if(ImageStartX<0) {
				ImageStartX = 0;
			}
			int ImageEndX = (ImageX + (int)ImageData.Width);
			if(ImageEndX > Buffer->Width-0){
				ImageEndX = Buffer->Width-0;
			}
			
			int StartDrawingX=0;
			if(ImageX<0) {
				StartDrawingX = 0 - ImageX;
			}
			int StartDrawingY=0;
			if(ImageY<0) {
				StartDrawingY = 0 - ImageY;
			}
		
			uint8 *BufferRow = (uint8*)Buffer->Memory;
			uint8 *BMPRow = (uint8*)ImageData.ImageData;
			uint32 ImagePitch = ImageData.Width*ImageData.BytesPerPixel;
			uint32 DIBPixel;
			BMPRow += StartDrawingX*ImageData.BytesPerPixel;
			BMPRow += StartDrawingY*ImagePitch;
			BufferRow += ImageStartX*Buffer->BytesPerPixel;
			BufferRow += ImageStartY*Buffer->Pitch;
			
			uint32 UseHeight = ImageData.Height;
			uint32 UseWidth = ImageData.Width;
			if(UseHeight>Buffer->Height) {
				UseHeight = Buffer->Height;
			}
			if(UseWidth>Buffer->Width) {
				UseWidth = Buffer->Width;
			}
			for(int Y = ImageStartY; Y<ImageEndY; ++Y){
				uint32 *BMPPixel = (uint32*)BMPRow;
				uint32 *Pixel = (uint32 *)BufferRow;
				for(int X = ImageStartX; X < ImageEndX; ++X) { //how much of row is drawn
					DIBPixel = *BMPPixel;
					if(DIBPixel!=BACKGROUND_COLOR ){
						*Pixel = DIBPixel;		
					}
					*BMPPixel++; // Go to next Pixel in BMP row
					*Pixel++;    // Go to next Pixel in SCREEN row
				}
				BMPRow += ImagePitch;      // Go to next row in BMP IMAGE
				BufferRow += Buffer->Pitch; // Go to next row on SCREEN
			}
			
		}
}


static bool GlobalRunning;
static win32_offscreen_buffer GlobalBackBuffer;
static bool GlobalLockMouse;
static int64 GlobalPerfCountFrequency;
static bool GlobalUnhighlighted;

// This function will make it easier to think about what we're doing when
// we're measuring time.
inline LARGE_INTEGER Win32GetWallClock (void) {
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return (Result);
}

// This function will make it easier to see how many seconds have elapsed
inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End){
	real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency);
	return (Result);
}

// FULL SCREEN SUPPORT
// This uses code from Raymond Chen's blogpost here:
// https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
static bool GlobalFullScreen = false;
static WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };
void ToggleFullScreen(HWND Window)
{
  DWORD dwStyle = GetWindowLong(Window, GWL_STYLE);
  if (dwStyle & WS_OVERLAPPEDWINDOW) {
    MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
    if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
        GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo)) {
		GlobalFullScreen = true;
		SetWindowLong(Window, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
		SetWindowPos(Window, HWND_TOP,
					 MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
				  	 MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
					 MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
					 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		int BytesPerPixel = 4;
		int ScreenWidth = MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left;
		int ScreenHeight = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;
		win32_offscreen_buffer * PointerToBackBuffer = &GlobalBackBuffer;
		PointerToBackBuffer->Width = ScreenWidth;
		PointerToBackBuffer->Height = ScreenHeight;  
		PointerToBackBuffer->Pitch = ScreenWidth*BytesPerPixel;
		PointerToBackBuffer->BytesPerPixel = BytesPerPixel;

		/* Set the DIB properties
		// A DIB is like a BMP, datawise. The BITMAPINFO is a struct that contains stuff about that DIB.
		// Inside BITMAPINFO is the bmiHeader, which will be very much like the
		// header of a BMP in that it gives info about the format of the data, such as...*/
		PointerToBackBuffer->Info.bmiHeader.biSize = sizeof(PointerToBackBuffer->Info.bmiHeader); // the size of the header itself
		PointerToBackBuffer->Info.bmiHeader.biWidth = PointerToBackBuffer->Width; //the width of the DIB
		PointerToBackBuffer->Info.bmiHeader.biHeight = -PointerToBackBuffer->Height; //the height of the DIB (negative tells Windows to draw top-down, not bottom-up)
		PointerToBackBuffer->Info.bmiHeader.biPlanes = 1; // The amount of planes in the DIB
		PointerToBackBuffer->Info.bmiHeader.biBitCount = 32; // The amount of bits per color in the DIB
		PointerToBackBuffer->Info.bmiHeader.biCompression = BI_RGB; // the kind of compression the DIB will use (BI_RGB = No compression)

		// Here we are calculating the amount of memory we need in bytes to have in order to fill each pixel,
		// This will change if the width or height or bytesperpixel are changed
		int BitmapMemorySize = (PointerToBackBuffer->Width*PointerToBackBuffer->Height)*BytesPerPixel;
		
		//If there is previous memory for the screen buffer, unallocate it
		if(PointerToBackBuffer->Memory){
			VirtualFree(PointerToBackBuffer->Memory, 0, MEM_RELEASE);
		}
		
		// Allocate the amount of memory we calculated
		PointerToBackBuffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    }
  } else {
	GlobalFullScreen = false;
    SetWindowLong(Window, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(Window, &GlobalWindowPosition);
    SetWindowPos(Window, 0, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}


LRESULT CALLBACK Win32MainWindowCallback(
    HWND   Window,
    UINT   Message,
    WPARAM WParam, // extra stuff
    LPARAM LParam  // extra stuff
) {
    LRESULT Result = 0;

    switch(Message){
        case WM_SIZE:
        {
        } break;
        case WM_DESTROY:
        {
            GlobalRunning = false;
			PostQuitMessage(0);
        } break;
		case WM_ACTIVATE:
        {
			if(WParam == WA_INACTIVE) {
				GlobalLockMouse = false;
			} else {
				GlobalLockMouse = true;
			}
			
        } break;
        case WM_CLOSE:
        {
            GlobalRunning = false;
            PostQuitMessage(0);
        } break;
		// WM_PAINT is the event that happens when the window starts to draw. It will rehappen every time
		// the user messes with the window (resizes it, moves it around, unminimizes it, etc)
        case WM_PAINT:{
            // Get a pointer to global backbuffer
			win32_offscreen_buffer * PointerToBackBuffer = &GlobalBackBuffer;

			PAINTSTRUCT Paint;		
            HDC DeviceContext = BeginPaint(Window, &Paint);
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			StretchDIBits(DeviceContext,
							0, 0, Width, Height,
							0, 0, PointerToBackBuffer->Width, PointerToBackBuffer->Height,
							PointerToBackBuffer->Memory,
							&PointerToBackBuffer->Info,
							DIB_RGB_COLORS,
							SRCCOPY
							);
			#if 0
			#endif
            EndPaint(Window, &Paint);

        } break;
        default:
        {
            //DefWindowsProc just returns the default windows procedure that would happen
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

// v THIS IS THE START OF THE PROGRAM v
int CALLBACK WinMain(
    // HINSTANCE - Instance handle of the application. Windows is set up to allocate us
    // our own virtual memory space. The HINSTANCE is the way we tell Windows that we
    // want to access addresses within that virtual memory
    HINSTANCE Instance,
    HINSTANCE PrevInstance, // null
    LPSTR CmdLine, // if any parameters are put in command line when the program is run
    int ShowCode // determines how app window will be displayed
) {
	//::ShowWindow(::GetConsoleWindow(), SW_HIDE);
	//::ShowWindow(::GetConsoleWindow(), SW_SHOW);
	
	// LOAD THE BMP IMAGES
	// ---------------
	// ---------------
	bmp_image_data waffle = GetGenericImageData("images\\01.png");
	bmp_image_data BigAnimeImage = GetBmpImageData("images\\Brand-New-Animal.bmp");
	bmp_image_data Image0 = GetBmpImageData("images\\00.bmp");
	bmp_image_data Image1 = GetBmpImageData("images\\01.bmp");
	bmp_image_data Image2 = GetBmpImageData("images\\02.bmp");
	bmp_image_data Image3 = GetBmpImageData("images\\03.bmp");
	bmp_image_data Image4 = GetBmpImageData("images\\04.bmp");
	bmp_image_data Image5 = GetBmpImageData("images\\05.bmp");
	bmp_image_data Image6 = GetBmpImageData("images\\06.bmp");
	bmp_image_data Image7 = GetBmpImageData("images\\07.bmp");
	bmp_image_data Image8 = GetBmpImageData("images\\08.bmp");
	bmp_image_data Image9 = GetBmpImageData("images\\09.bmp");
	bmp_image_data Image10 = GetBmpImageData("images\\10.bmp");
	
	// This is data for FPS and the like
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	
	// SET UP TIMING
	// ---------------
	// ---------------
	// We will need a counter that will count the current time
	LARGE_INTEGER LastCounter = Win32GetWallClock();
	LARGE_INTEGER FlipWallClock = Win32GetWallClock();
	HMODULE TimeBeginPeriodLibrary = LoadLibraryA("winmm.dll");
	if(TimeBeginPeriodLibrary){
		std::cout << "got it\n";
		timeBeginPeriod = (time_begin_period *)GetProcAddress(TimeBeginPeriodLibrary, "timeBeginPeriod");
	}
	bool32 SleepIsGranular = false;
	UINT DesiredSchedulerMS = 1;
	MMRESULT period = timeBeginPeriod(DesiredSchedulerMS);
	//std::cout << "MMRESULT:" << TIMERR_NOCANDO << "\n";
	if(period==TIMERR_NOERROR) {
		SleepIsGranular = true;
	}
	
	// SET UP THE DIB
	// ---------------
	// ---------------
	// Get a pointer to global backbuffer
	win32_offscreen_buffer * PointerToBackBuffer = &GlobalBackBuffer;

	// Declare variables for the window width and height
	int WindowWidth;
	int WindowHeight;

	// We allocated a void pointer to some memory. That memory may currently have something in it from some previous process,
	// so clear the memory to stop that something from messing with our stuff
	if(PointerToBackBuffer->Memory){
        VirtualFree(PointerToBackBuffer->Memory, 0, MEM_RELEASE);
    }

	// Set the screen properties (this could probably be set elsewhere, but here it is for now)
	int ScreenWidth = 960;
	int ScreenHeight = 540;
	int BytesPerPixel = 4;
    PointerToBackBuffer->Width = ScreenWidth;
    PointerToBackBuffer->Height = ScreenHeight;
    PointerToBackBuffer->Pitch = ScreenWidth*BytesPerPixel;
	PointerToBackBuffer->BytesPerPixel = BytesPerPixel;

	// Set the DIB properties
	// A DIB is like a BMP, datawise. The BITMAPINFO is a struct that contains stuff about that DIB.
	// Inside BITMAPINFO is the bmiHeader, which will be very much like the
	// header of a BMP in that it gives info about the format of the data, such as...
	PointerToBackBuffer->Info.bmiHeader.biSize = sizeof(PointerToBackBuffer->Info.bmiHeader); // the size of the header itself
    PointerToBackBuffer->Info.bmiHeader.biWidth = PointerToBackBuffer->Width; //the width of the DIB
    PointerToBackBuffer->Info.bmiHeader.biHeight = -PointerToBackBuffer->Height; //the height of the DIB (negative tells Windows to draw top-down, not bottom-up)
    PointerToBackBuffer->Info.bmiHeader.biPlanes = 1; // The amount of planes in the DIB
    PointerToBackBuffer->Info.bmiHeader.biBitCount = 32; // The amount of bits per color in the DIB
    PointerToBackBuffer->Info.bmiHeader.biCompression = BI_RGB; // the kind of compression the DIB will use (BI_RGB = No compression)
	// Here we are calculating the amount of memory we need in bytes to have in order to fill each pixel,
	// This will change if the width or height or bytesperpixel are changed
    int BitmapMemorySize = (PointerToBackBuffer->Width*PointerToBackBuffer->Height)*BytesPerPixel;

    // Allocate the amount of memory we calculated
    PointerToBackBuffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);


	// OPEN A WINDOWS WINDOW
	// ---------------
	// ---------------
    // Allocate space for a blank Window class on the stack
    WNDCLASSA WindowClass = {};

    // Add necessary properties to the Window class
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;

	// This is a reference to that LRESULT CALLBACK Win32MainWindowCallback function we had to create above
    WindowClass.lpfnWndProc = Win32MainWindowCallback;

	//WindowClass.cbWndExtra;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon;
    WindowClass.lpszClassName = "ScreenTestWindowClass";

    // regeister the Window class
    if(RegisterClassA(&WindowClass)){

        // create a window handle, this will create our very own window that we can use for whatever we want
        HWND Window = CreateWindowExA(
            0,                              //dwExstyle
            WindowClass.lpszClassName,      //lpClassName
            "Screen Test",                //lpWindowName - name of window
            WS_OVERLAPPEDWINDOW|WS_VISIBLE, //dwStyle
            CW_USEDEFAULT,                  //X
            CW_USEDEFAULT,                  //Y
            ScreenWidth,                  //nWidth
            ScreenHeight,                  //nHeight
            0,                              //hWndParent - 0 if only one window
            0,                              //hMenu 0 - if not using any windows menus
            Instance,                       //hInstance - the handle to the app instance
            0                               //lpParam
        );

        if(Window){
            int MonitorRefreshHz = 60;
			HDC DeviceContext = GetDC(Window);
			int Win32RefreshRate = GetDeviceCaps(DeviceContext, VREFRESH);
			
			if(Win32RefreshRate>1){
				MonitorRefreshHz = Win32RefreshRate;
			}
			static double GameUpdateHz (MonitorRefreshHz / 2.0f);
			GameUpdateHz = 60;
			double TargetSecondsPerFrame = 1.0f / (double)GameUpdateHz;

			// Set up variables to make the weird gradient move
			int XOffset = 0;
            int YOffset = 0;
			int XSpeed = 0;
			int YSpeed = 0;
			int Speed = 2;

            GlobalRunning = true;
			
			// Put mouse in center of screen;
			RECT ClientRect;
			
			GetClientRect(Window, &ClientRect);

			uint32 MouseWindowWidth = ClientRect.right - ClientRect.left;
			uint32 MouseWindowHeight = ClientRect.bottom - ClientRect.top;
			
			SetCursorPos(ClientRect.right - MouseWindowWidth/2, ClientRect.bottom - MouseWindowHeight/2 );
			
			POINT OldCursorPosition;
			GetCursorPos(&OldCursorPosition);
			POINT NewCursorPosition = OldCursorPosition;
			POINT StartCursorPosition = OldCursorPosition;
			int MouseDeltaX = 0;
			int MouseDeltaY = 0;
			GlobalLockMouse = true;
			
			// MAIN LOOP
			// ---------------
			// ---------------
			// ---------------
			while(GlobalRunning){
				// Allocate enough memory for a MSG structure, so we can place the stuff GetMessage()
                // spits out into it
                 MSG Message;

                // HANDLE KEYBOARD MESSAGES
				// ---------------
				// ---------------
                while(PeekMessage(&Message,0,0,0,PM_REMOVE)){
                    if(Message.message == WM_QUIT){
                        GlobalRunning = false;
                    }
					switch(Message.message) {
						case WM_SYSKEYDOWN:
						case WM_SYSKEYUP:
						case WM_KEYDOWN:
						case WM_KEYUP:{
							uint32 VKCode = (uint32)Message.wParam;
							bool WasDown = ((Message.lParam & (1 << 30)) != 0);
							bool IsDown = ((Message.lParam & (1<< 31)) == 0);
							if(WasDown != IsDown) {
								if(VKCode == 'W') {

								} else if (VKCode == 'A') {
								} else if (VKCode == 'S') {
								} else if (VKCode =='D') {
								} else if (VKCode == 'Q') {
								} else if (VKCode == 'E') {
								} else if (VKCode == VK_UP) {
									if(WasDown)
										YSpeed=0;
									if(IsDown)
										YSpeed=-Speed;
								} else if (VKCode == VK_DOWN) {
									if(WasDown)
										YSpeed=0;
									if(IsDown)
										YSpeed=Speed;
								} else if (VKCode == VK_RIGHT) {
									if(WasDown)
										XSpeed=0;
									if(IsDown)
										XSpeed=Speed;
								} else if (VKCode == VK_LEFT) {
									if(WasDown)
										XSpeed=0;
									if(IsDown)
										XSpeed=-Speed;
								} else if (VKCode == VK_ESCAPE) {
									if(IsDown) {
										if(GlobalFullScreen==true) {
											ToggleFullScreen(Window);
										} else {	
											GlobalRunning = false;										
										}
									}
								} else if (VKCode == VK_F4) {
									if(IsDown)
										ToggleFullScreen(Window);
								} else if (VKCode == VK_RETURN) {
									bool AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
									if(WasDown) {
										if(AltKeyWasDown) {
											ToggleFullScreen(Window);
										}
									}
								}
								bool AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
								if ((VKCode == VK_F4) && AltKeyWasDown){
									GlobalRunning = false;
									PostQuitMessage(0);
								}
							}

						} break;

						default: {
								TranslateMessage(&Message);
								DispatchMessage(&Message);
						} break;
					}
                }
				
				// HANDLE MOUSE INPUT
				// ---------------
				// ---------------
				if(GlobalLockMouse==true) {
					ShowCursor(false);
					GetCursorPos(&NewCursorPosition);
					MouseDeltaX = (OldCursorPosition.x-NewCursorPosition.x)*-1;
					MouseDeltaY = (OldCursorPosition.y-NewCursorPosition.y)*-1;
					NewCursorPosition.x = StartCursorPosition.x;
					NewCursorPosition.y = StartCursorPosition.y;
					OldCursorPosition = NewCursorPosition;
					SetCursorPos(StartCursorPosition.x,StartCursorPosition.y);
					//std::cout << "MouseX:" << MouseDeltaX << "\nMouseY:" << MouseDeltaY << "\n";
				} else {
					ShowCursor(true);
				}
				
				// RENDER BACKGROUND AND IMAGES
				// ---------------
				// ---------------
				// Black Background
				if(GlobalLockMouse) {
						uint32 *BlackBackgroundPixel = (uint32*)GlobalBackBuffer.Memory;
					for(int i = 0; i<(GlobalBackBuffer.Width*GlobalBackBuffer.Height); i++) {
						*BlackBackgroundPixel++ = 0;
					}
					
					XOffset -= MouseDeltaX;
					YOffset -= MouseDeltaY;
					
					// Images
					int XExtra = 0;
					int YExtra = 0;
					DrawImage(BigAnimeImage, XOffset, YOffset, &GlobalBackBuffer);
					YExtra += BigAnimeImage.Height/1.5;
					XExtra += BigAnimeImage.Width;
					DrawImage(Image0, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image0.Height/1.5;
					XExtra += Image0.Width;
					DrawImage(Image1, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image1.Height/1.5;
					XExtra += Image1.Width;
					DrawImage(Image2, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image2.Height/1.5;
					XExtra += Image2.Width;
					DrawImage(Image2, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image3.Height/1.5;
					XExtra += Image3.Width;
					DrawImage(Image4, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image4.Height/1.5;
					XExtra += Image4.Width;
					DrawImage(Image5, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image5.Height/1.5;
					XExtra += Image5.Width;
					DrawImage(Image6, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image6.Height/1.5;
					XExtra += Image6.Width;
					DrawImage(Image7, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image7.Height/1.5;
					XExtra += Image7.Width;
					DrawImage(Image8, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image8.Height/1.5;
					XExtra += Image9.Width;
					DrawImage(Image9, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image9.Height/1.5;
					XExtra += Image9.Width;
					DrawImage(Image10, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					YExtra += Image10.Height/1.5;
					XExtra += Image10.Width;
					DrawImage(waffle, XOffset+XExtra, YOffset+YExtra, &GlobalBackBuffer);
					
				}
				
				// DISPLAY BUFFER IN WINDOW
				// ---------------
				// ---------------
				if(GlobalLockMouse) {
					RECT ClientRect;

					GetClientRect(Window, &ClientRect);
					
					WindowWidth = ClientRect.right - ClientRect.left;
					WindowHeight = ClientRect.bottom - ClientRect.top;

					StretchDIBits(DeviceContext,
									0, 0, WindowWidth, WindowHeight,
									0, 0, WindowWidth, WindowHeight,
									PointerToBackBuffer->Memory,
									&PointerToBackBuffer->Info,
									DIB_RGB_COLORS,
									SRCCOPY
									);	
				}
						
				// LIMIT THE FPS (TODO: explain)
				// ---------------
				// ---------------
				LARGE_INTEGER WorkCounter = Win32GetWallClock();
				double SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, WorkCounter);
				
				if(SecondsElapsedForFrame < TargetSecondsPerFrame) {
					if(SleepIsGranular) {
						DWORD SleepMS = (DWORD)(800.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
						if(SleepMS > 0){
							Sleep(SleepMS);								
						}
					}
					#if 0
					#endif
					while(SecondsElapsedForFrame < TargetSecondsPerFrame) {
						SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
					}
				LARGE_INTEGER TestCounter = Win32GetWallClock();
				double SecondsElapsedSinceSleep = Win32GetSecondsElapsed(WorkCounter, TestCounter);					
				} else {
					std::cout << "missed frame\n";
					// Missed frame rate logging
				}
				
				// FPS Tracking
				double FramesPerSecond;
				double SecondsPerFrame = Win32GetSecondsElapsed(LastCounter,Win32GetWallClock());
				FramesPerSecond = 1.0f/ SecondsPerFrame;
				//std::cout << "FPS: " << FramesPerSecond << "\n";
				LARGE_INTEGER EndCounter = Win32GetWallClock();
				LastCounter = EndCounter;
            }
        } else {
            // TODO(Casey): Logging
        }
    } else {
        // TODO(Casey): Logging
    }

    return(0);
}