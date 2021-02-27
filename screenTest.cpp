// We want to be explicit about the number of bytes we're allocating in our variables, so we're
// importing more explicit definitions
#include <stdint.h>

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

#define Pi32 3.14159265359f
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <malloc.h>
#include <limits>

// This will allow you to make windows create a window for you
#include <windows.h>

// This will allow you to use Microsoft's direct sound definitions
#include "mingwdsound.h"

// This will allow you to use Microsoft's Xinput definitions
#include "mingwxinput.h"
//#include <dsound.h>

#include "ScreenTest.h"

struct win32_offscreen_buffer {
    // NOTE(casey): Pixels are always 32 bits wide, Memory Order BB GG RR XX

    BITMAPINFO Info; // it's not really the whole BITMAPINFO we care about, just the header
    void *Memory; 	// this be the memory where the data of the DIB is stored;
	// the ^ variable is not the memory itself, but a pointer to the location of the memory
    int Width;
    int Height;
    int Pitch;
};

static bool GlobalRunning;

// Screen Buffer
static win32_offscreen_buffer GlobalBackBuffer;

// Sound Buffer
static LPDIRECTSOUNDBUFFER GlobalSoundBuffer;

static void *GlobalDibMemory;

static int64 GlobalPerfCountFrequency;

/* MAKE XInputGetState() SAFE TO USE 
// We want to be able to call the XInputGetState() function without getting any
// errors, even if we aren't able to import the library that contains the 
// definition of this function.

// We will accomplish this by using a function pointer.
 
// A function pointer is basically a special function that's empty of any 
// execution code. To give it execution code, you need to point it to a normal 
// function that has the same function signature, i.e. the same input variables,
// and it will execute the code in that normal function as if it were its own
// code. The cool thing about a function pointer is that you can change the 
// function that it points to at any time. 

// What we will do is create a function pointer, point it to a fake 
// XInputGetState() function that does basically nothing, then trick 
// the C++ compiler into thinking that its previously declared
// XInputGetState() function is actually our function pointer. We'll 
// do this in 4 easy steps.
*/

/* Step 1 - Create a Function Type (x_input_get_state) with the same signature 
// (DWORD dwUserIndex, XINPUT_STATE *pState) as the XInputGetState() function.
// This function type will be used to create the Function Pointer.*/ 
typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);

/* Step 2 - Define a fake XInputGetState() function.
// This function will basically do nothing.*/
DWORD WINAPI XInputGetStateStub(DWORD dwUserIndex, XINPUT_STATE *pState) {
	return(ERROR_DEVICE_NOT_CONNECTED);
}

/* Step 3 - Create a function pointer and point it to our fake XInputGetState()
// function.
// We'll name this pointer XInputGetState_ because XInputGetState is already
// taken.*/
static x_input_get_state *XInputGetState_ = XInputGetStateStub;

/* Step 4 - Trick the C++ compiler into using our XInputGetState_ function
// pointer whenever we call the XInputGetState() function.*/
#define XInputGetState XInputGetState_
/* later, we will actually point this function pointer directly to the 
// XInputGetState() function located in the XInput DLL. This will make it so
// we don't have to import the Lib file in our code.*/


/* MAKE XInputSetState() SAFE TO USE
// This is a shorter way to do the same thing we did for XInputGetState() 
// see "MAKE XINPUTGETSTATE() SAFE TO USE" above for a long explanation.
// This #define basically tells the compiler that whenever we type 
// X_INPUT_SET_STATE(WhateverNameWeWantOurFunctionToBe), It will take that
// text we typed and replace it with 
// DWORD WINAPI WhateverNameWeWantOurFunctionToBe(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)*/
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
	return(ERROR_DEVICE_NOT_CONNECTED);
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


#define TIME_BEGIN_PERIOD(name) MMRESULT name(UINT uPeriod)
typedef TIME_BEGIN_PERIOD(time_begin_period);
TIME_BEGIN_PERIOD(TimeBeginPeriodStub){
	return TIMERR_NOCANDO;
}
static time_begin_period *timeBeginPeriod_ = TimeBeginPeriodStub;
#define timeBeginPeriod timeBeginPeriod_


//winmm.dll

//CopyFile("PrintSomethingCool.dll", "PrintSomethingCool_Temp.dll", FALSE); 
// For testing DLL import
typedef void print_something_cool();
void PrintSomethingCoolStub () {
	std::cout << "DLL import for PrintSomethingCool() doesn't work.\n";
}
typedef void game_update_and_render(game_offscreen_buffer *Buffer, game_state *GameState, game_controller_input *Controller,
									game_sound_output_buffer *SoundBuffer);
void GameUpdateAndRenderStub (game_offscreen_buffer *Buffer,  game_state *GameState, game_controller_input *Controller, 
							  game_sound_output_buffer *SoundBuffer) {
	std::cout << "DLL import for GameUpdateAndRender() doesn't work.\n";
}
typedef void game_update_sound(game_sound_output_buffer *SoundBuffer);
void GameUpdateSoundStub (game_sound_output_buffer *SoundBuffer) {
	std::cout << "DLL import for GameUpdateSound() doesn't work.\n";
}

/* Create a Function Type that has the same signature as the direct_sound_create function
// in the DirectSound Library. 
// We will use said
// Function Type to create a Function Pointer named DirectSoundCreate, which
// we will point to the DirectSoundCreate() function located in dsound.dll.
// This is totally hacky way to avoid having to use the DirectSound Library
// by instead importing only the function we need from the DLL file
// located somewhere in Windows's innards. This will obviously only work on 
// Windows, but it's still pretty rad.
*/
typedef HRESULT WINAPI direct_sound_create(LPCGUID pcGuideDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);

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

/* ( v This is NOT the start of the program. v It is a function that windows REQUIRES if you
// want to open a window. 
//When you create a window class (WNDCLASSA), one of its properties
// will be a reference to this function )
// This function is here so it can be called when something happens to the window
// If something happens to a window, it will send a Message to this callback
// eg. messages = Window resized (WM_SIZE),window clicked on (WM_ACTIVATEAPP), etc.
// LRESULT CALLBACK is a Long Pointer that points to the start of this function
*/
LRESULT CALLBACK Win32MainWindowCallback(
    HWND   Window,
    UINT   Message,
    WPARAM WParam, // extra stuff
    LPARAM LParam  // extra stuff
) {
    LRESULT Result = 0;

    // Depending on the event that happens we want to do different things
    switch(Message){
        case WM_SIZE:
        {
        } break;
        case WM_DESTROY:
        {
            GlobalRunning = false;
			PostQuitMessage(0);
        } break;
        case WM_CLOSE:
        {
            GlobalRunning = false;
            PostQuitMessage(0);
        } break;
		// WM_PAINT is the event that happens when the window starts to draw. It will rehappen every time
		// the user messes with the window (resizes it, moves it around, unminimizes it, etc)
        case WM_PAINT:{
			std::cout << "WM_PAINT" << "\n";
            // Get a pointer to global backbuffer
			win32_offscreen_buffer * PointerToBackBuffer = &GlobalBackBuffer;

			// Allocate enough memory for a paintstruct
			PAINTSTRUCT Paint;

			// Get the DeviceContext by pointing Windows's BeginPaint function to that PAINTSTRUCT
            HDC DeviceContext = BeginPaint(Window, &Paint);

			// RECT is the data format that Windows uses to represent the dimensions of
			// a window (HWND) that has been opened.
			// v This command v allocates the amount of space needed for that RECT on the stack
			RECT ClientRect;

			// v This v is a windows.h command that will get the RECT of any given window
			// However, instead of RETURNING the value like a normal function would do, this function shoves the value into an already allocated space of memory
			// (in our case this space of memory would be the one that was created by our ^ RECT ClientRect; ^ command above)
			GetClientRect(Window, &ClientRect);

			// Now we're getting the width and height (which is all we want) from that RECT
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;

			// DISPLAY BUFFER IN WINDOW, again
			// ---------------
			// ---------------
			// StretchDIBits takes the color data (what colors the pixels are) of an image
			// And puts it into a destination. In this case, that destination will be the back
			// buffer
			StretchDIBits(DeviceContext,
							0, 0, PointerToBackBuffer->Width, PointerToBackBuffer->Height,
							0, 0, PointerToBackBuffer->Width, PointerToBackBuffer->Height,
							PointerToBackBuffer->Memory,
							&PointerToBackBuffer->Info,
							DIB_RGB_COLORS,
							SRCCOPY
							);
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
    /* HINSTANCE - Instance handle of the application. Windows is set up to allocate us
    // our own virtual memory space. The HINSTANCE is the way we tell Windows that we
    // want to access addresses within that virtual memory*/
    HINSTANCE Instance,
    HINSTANCE PrevInstance, // null
    LPSTR CmdLine, // if any parameters are put in command line when the program is run
    int ShowCode // determines how app window will be displayed
) {
	// Hide the console (Windows XP shows the console even though this is a WinMain function when it's compiled with MinGW)
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
	
	
	// Filenames for hot reloading
	char SourceGameCodeDLLFullPath[MAX_PATH] = "C:\\Users\\goken\\cppProjects\\HandmadeHero\\ScreenTest.exe";
	char TempGameCodeDLLFullPath[MAX_PATH] = "C:\\Users\\goken\\cppProjects\\HandmadeHero\\ScreenTest_temp.exe";
	
	
	// Declare a function pointer to a game_update_and_render type function
	game_update_and_render *GameUpdateAndRender;
	// Point that function pointer to our fake GameUpdateAndRenderStub function
	GameUpdateAndRender = &GameUpdateAndRenderStub;
	
	// Declare a function pointer to a game_update_sound type function
	game_update_sound *GameUpdateSound;
	// Point that function pointer to our fake GameUpdateSoundStub function
	GameUpdateSound = &GameUpdateSoundStub;
	
	// This is data for FPS and the like
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	
	// TODO: explain
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
	//bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
	
	//timeBeginPeriod(1);
	
	// SET UP THE GAMEPAD
	// ---------------
	// ---------------
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if(!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("XInput9_1_0.dll");
	}
	if(!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if(XInputLibrary){
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	} else {
		
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
	int WindowsWindowWidth = 1280;
	int WindowsWindowHeight = 720;
	int BytesPerPixel = 4;
    PointerToBackBuffer->Width = ScreenWidth;
    PointerToBackBuffer->Height = ScreenHeight;  
    PointerToBackBuffer->Pitch = ScreenWidth*BytesPerPixel;

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
            WindowsWindowWidth,                  //nWidth
            WindowsWindowHeight,                  //nHeight
            0,                              //hWndParent - 0 if only one window
            0,                              //hMenu 0 - if not using any windows menus
            Instance,                       //hInstance - the handle to the app instance
            0                               //lpParam
        );
		
		// If we succed at opening the Windows window, we will start initializing all our
		// game stuff
        if(Window){
			// This is the framerate we will try to hit
			int MonitorRefreshHz = 60;
			HDC RefreshDC = GetDC(Window);
			int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
			ReleaseDC(Window, RefreshDC);
			
			if(Win32RefreshRate>1){
				MonitorRefreshHz = Win32RefreshRate;
			}
			static double GameUpdateHz (MonitorRefreshHz / 2.0f);
			double TargetSecondsPerFrame = 1.0f / (double)GameUpdateHz;		

			// INITIALZE SOUND
			// ---------------
			// ---------------
			// EXPLANATION OF HOW SOUND WORKS
			{
			// Fundamentally, digital sound is just a bunch of points set up in such a way 
			// that if you connect them with a line, they will draw a wave:
			/*  .   .   .
			   . . . . . .
			  .   .   .   .
			*/
			
			// These points are called samples. Each sample is actually not a point in space. It's 
			// just the height of the point in space.
			/* 3.  3.  3.
			  2.2.2.2.2.2.
			 1.  1.  1.  1.
			*/
			
			// So really, if you look at the actual data that represents sound, it would just be a
			// bunch of numbers, one after the other that represent the individual heights of the
			// sound wave:
			/*
			 [1232123212321]
			*/
			
			// The computer's sound card will start at the beginning of this data:
			/*
		(Sound Card)
		    |     
		    v 
			[1232123212321]
			*/
			
			// And it will move through the data, one sample at a time, generating it into a wave:
			/*
		  ----------->(Sound Card)
					      |    
				          |
			[123 21 23 21 23 21]
			              |
			   3/\   3/\  v 
			  2/ 2\ 2/ 2\ 2/  
			 1/   1\/   1\/ 
			 
			 */
			 
			 // As it generates this wave, the sound hardware will oscillate at the frequencey
			 // of this wave, and we will hear sound (if speakers or headphones are plugged in to 
			 // the Sound Card, obviously):
			 /*
			   /\    /\    /\
			  /  \  /  \  /  \
			 /    \/    \/    \
			   ^			  
			  This particular wave would sound like a high pitched hum.
			*/
			
			// Now for where we write this data to get the Sound Card to read it:
			// The Sound Card will only read data in a special space in memory called the Sound Buffer.
			// The Sound Buffer used to be a literal address of memory located in the Sound Card itself 
			// that you could write directly into, but nowadays it has been virutualized, so you have to 
			// ask Windows for a fake memory address that you can write into. Windows will then copy the
			// memory from this fake address over to the actual physical address on the Sound Card. 
			// There are several ways you can ask Windows for access to this virtual sound
			// buffer, but the easiest is through an old Windows Library called DirectSound. We are
			// going to use that library.
			
			// But before we start DirectSound, we're gonna set some values we will use to do so:
			}
			
			
			/* The speed at which this wave is read is called the SamplesPerSecond, it's also known as 
			// the hz (48000hz, 44100hz, 22000hz, etc)
			// The higher the SamplesPerSecond, the more data you can fit into a second in sound, and 
			// the higher quality your sound will be.
			// We'll set our sound's SamplesPerSecond to 48000hz, since that's the standard for high
			// quality audio*/
			int SamplesPerSecond = 48000;
			
			/* Each sample is a particular height at a particular time. That height
			// will represent the volume of that sound at that time. The BytesPerSample is how 
			// granularly you can set that height. We are going to set our BytesPerSample to 2 bytes, 
			// which is the size of an int16. Since our sound is going to be stereo, we will need it 
			// to be the size of 2 int16s, which will be 4 bytes total.*/
			int BytesPerSample = sizeof(int16)*2;
			
			/* The RunningSampleIndex is a variable we're creating to represent the individual sample 
			// that the sound card is currently on in our data when it's generating the wave*/
			uint32 RunningSampleIndex = 0; 
			
			
			/* SoundBufferSize = how big we want our SoundBuffer to be. We are going to make it as big as 
			// one second of sound.*/
			DWORD SoundBufferSize = SamplesPerSecond*BytesPerSample;
			
			/* There is always some variability in the timing of the actual hardware we're writing to, so
			// we're going to add a small amount of bytes that will account for that variabliity and
			// prevent it from messing with our sound. We'll call these bytes the SafetyBytes.*/
			DWORD SafetyBytes = (SamplesPerSecond*BytesPerSample / GameUpdateHz) / 1;
			
			/* Alright, now we can import the library.
			// Instead of importing the DirectSound Lib file and linking it, which would be annoying 
			// in MinGW, we will just import the DLL. LoadLibraryA will tell Windows to search 
			// for the DLL in its inner libraries, I believe, but don't quote me on that.*/
			HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
			
			// We will only initialize DirectSound if Windows is able to find the library.
			if(DSoundLibrary){
				// Get a DirectSound object by grabbing it directly from the dsound DLL
				direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
			
				/* For Direct Sound, we need to initialize a Primary Buffer and a Secondary Buffer.
				// The Primary Buffer used to be a way to write data directly into to the sound card itself, 
				// but at some point, it was decided that writing directly into hardware wasn't something
				// that should be done anymore, so now the Primary Buffer is just used to set up the format
				// of the sound. 
				// The Secondary Buffer will go to the fake address Windows creates for you to virtually access
				// the sound card. When you put data into the Secondary Buffer, Windows will copy that data 
				// into the sound card for you.*/	
				LPDIRECTSOUND DirectSound;
				if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
					
					// Set up the format of the sound
					WAVEFORMATEX WaveFormat = {};
					WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
					WaveFormat.nChannels = 2;	// 2 channels = stereo sound
					WaveFormat.nSamplesPerSec = SamplesPerSecond;
					WaveFormat.wBitsPerSample = 16; // This will be the granularity of the volume of the sound. 16 will be 32 bits total because its stereo sound
					WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample)/8; 
					WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
					WaveFormat.cbSize = 0;
					if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))){
							
							// Create the Primary buffer (i.e. the buffer that just sets the format of the sound)
							DSBUFFERDESC BufferDescription = {};
							BufferDescription.dwSize = sizeof(BufferDescription);
							BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
							LPDIRECTSOUNDBUFFER PrimaryBuffer;
							if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer,0))) {						
								if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))){
										// The format has successfully been set at this point.
								} else {
									// This would be a good place for logging an error
								}
							}
					} else {
						// Error logging would go here
					}

					// Create the Secondary buffer (i.e. the buffer that we actually need to write into)
					DSBUFFERDESC BufferDescription = {};
					BufferDescription.dwSize = sizeof(BufferDescription);
					BufferDescription.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY;
					BufferDescription.dwBufferBytes = SoundBufferSize;
					BufferDescription.lpwfxFormat = &WaveFormat;
					
					HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSoundBuffer, 0);
					if(SUCCEEDED(Error)) {
							// The buffer we can write into has successfully been created
					} else {
						// Error logging
					}
				} else {
					// Error Logging
				}		
			}
			
			// THE SOUND BUFFER LOOP EXPLANATION 
			{
			/*
			THE SOUND BUFFER LOOP
			The ^ SoundBuffer that we created above ^ with the DirectSound->CreateSoundBuffer() method is a 
			special space in memory that the DirectSound allocates for us. 
			The computer's sound card will virtually read into this space of memory when it wants to output sound.
			Here's a diagram to represent this space in memory:
			
			|------------------------------------------------------------------------|
			
			
			Within the SoundBuffer there are two values we have to be aware of: the PlayCursor, and the 
			WriteCursor. Let's add them to our diagram:
			
			|[*]-------------------------{*}-----------------------------------------|
		  PlayCursor				  WriteCursor
			
			When you start the SoundBuffer playing with the SoundBuffer->Play() method, the PlayCursor and 
			the WriteCursor will start moving forward in the memory:
			
			|---------------[*]-------------------------{*}--------------------------|
			   --------->PlayCursor		  --------->WriteCursor
			
			And they will keep moving and moving and moving
			
			|----------------------------------------[*]-------------------------{*}-|
								   -------------->PlayCursor      ---------->WriteCursor
			
			If a Cursor goes farther than the end of the memory, it will loop back around to the beginning:
			
			|-------{*}----------------------------------------[*]-------------------|
		     --->WriteCursor		                  ------>PlayCursor            --
			   (looped around)
		
			And they will keep moving in this way pretty much forever
			
			|----------------{*}----------------------------------------[*]----------|
			 ------------>WriteCursor------------------------------->PlayCursor------           
			
			
			THE PLAYCURSOR AND WRITECURSOR
			The PlayCursor points to the spot in the SoundBuffer where the sound card is 
			currently reading the data to output the sound. To be more precise, it points to the
			address JUST AFTER that spot. 
			
			The WriteCursor is the closest spot in the memory ahead of the PlayCursor 
			where it is safe to write sound data. 
			
			If you write data ahead of the WriteCursor, your sound will sound good (oooo). 
			If you DON'T write data ahead of the WriteCursor, it will sound glitchy or 
			scratchy or wrong somehow (xxxxx) because the PlayCursor might hit some data 
			before it actually gets written or when it's only been half-written.
			
			                        (v bad sounding data v)  (v good sounding data v)
			|---------------[*]------xxxxxxxxxxxxxxxxxxxxx{*}ooooooooooooooooooooooo|
			   --------->PlayCursor		  --------->WriteCursor
			*/
			
			
			// CLEAR THE SOUND BUFFER
			/* 
			-------
			Before we start our sound playing with the SoundBuffer->Play() method,
			we want to make sure all of the data within it is cleard to 0 because
			sometimes Windows will allocate space for the data without actually
			clearing it, which will give us bad data.
			
			LOCKING THE SOUND BUFFER
			The SoundBuffer->Lock method will basically take a certain specified amount 
			of the SoundBuffer's memory and lock it for us, so we can write sound 
			data into it. It will then take some pointers you give it (*Region1, *Region2) 
			and point them to the starting address of that sound data we just locked.
			*/
			}
			
			// The first thing we need to do is create some pointers that will be
			// be used for the regions.
			VOID *Region1;
			DWORD Region1Size;
			VOID *Region2;
			DWORD Region2Size;
			
			// REGION1 and REGION2 EXPLANATION 
			{
			/*			
			If there is NO looping in the data we're trying to lock, *Region1 will 
			point to the starting address of all the data we have locked, and 
			*Region2 will be completely blank.
			
			If there IS looping in the data we're trying to lock, *Region1 will point
			to the starting address of the data BEFORE the looping happens, and 
			*Region2 will point to the starting address of the data AFTER the looping 
			happens.
			
			To illustrate this, let's go back to our good old SoundBuffer diagram:
			
			|-----[*]-------------------------{*}-----------------------------------|
		       PlayCursor				  WriteCursor
			   
			Let's say we want to lock (L) some data ahead of the WriteCursor, and the 
			amount of data happens to NOT have any looping. Here's what the regions
			would look like:
										(*Region1 v)  									(*Region2 v)                                
			|-----[*]--------------------------{*}LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL--|             0
		       PlayCursor				  WriteCursor
			   
			*Region1 points to the beginning of the locked data, and *Region2 points 
			nowhere because it's not needed.
			
			Now let's say we want to lock (L) an amount of data ahead of the 
			WriteCursor that DOES have looping. Here's what the regions would look
			like for that:
			
   (*Region2 v)					        (*Region1 v)
			|LLLLL[*]--------------------------{*}LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL|
		       PlayCursor				  WriteCursor
			   
			  *Region1 points to the beginning of the locked data BEFORE the looping,
			  and *Region2 points to the beginning of the locked data AFTER the
			  looping. See? Easy peasy.
			  
			  Region1Size and Region2Size specify the size of the data locked, and
			  you would use them if you wanted to loop through the data without going
			  beyond it.
			*/
			}
			
			
			// Alright, let's shove these pointers into the Lock method, and get our
			// SoundBuffer address, so we can clear the whole thing
			if(SUCCEEDED(GlobalSoundBuffer->Lock(0, SoundBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0 ))) {
				/* Since we're basically grabbing all of the data from 0, this will,
				// in theory, only have to use *Region1, but we're going to loop
				// through both regions just in case*/
				uint8 *DestSample = (uint8 *)Region1; 
				for(DWORD ByteIndex=0; ByteIndex < Region1Size; ++ByteIndex) {
					*DestSample++ = 0; // Clear to 0
				}
				DestSample = (uint8 *)Region2;
				for(DWORD ByteIndex=0; ByteIndex < Region2Size; ++ByteIndex) {
					*DestSample++ = 0; // Clear to 0
				}
				GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
			}
			
			// Now we can start playing the sound knowing for sure that it won't 
			// make random noises because of uncleared data.
			GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);	
			
			/* ALLOCATE PRE-WRITE SOUND GENERATION MEMORY FOR SOUND BUFFER
			// This is the memory we are going to use to generate some of our own sound before 
			// it gets written into the SoundBuffer memory*/
			int16 *Samples = (int16 *)VirtualAlloc(0, SoundBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			
			/* This is just a way we will track if it's our first time through the sound loop, as we will
			// do things slightly different in that case. Once we've hit our first stime through the loop
			// we will set this value to true*/ 
			bool SoundIsValid = false;
	
			// INITIALIZE VIDEO
			// ---------------
			// ---------------
            /* The Device Context is the place in memory where the drawing will go.
			// v This function v will temporarily grab the Device Context of the Window we've just created, so we can draw into it*/
            HDC DeviceContext = GetDC(Window);

			// Set up variables to make the weird gradient move
			int XOffset = 0;
            int YOffset = 0;
			int XSpeed = 0;
			int YSpeed = 0;
			int Speed = 5;
			int ToneHz = 256;
			
			// SET UP TIMING
			// ---------------
			// ---------------
			// We will need a counter that will count the current time
			LARGE_INTEGER LastCounter = Win32GetWallClock();
			LARGE_INTEGER FlipWallClock = Win32GetWallClock();
			
			
			// Import the DLL with our actual PrintSomethingCool function
			HMODULE GameCodeDLL = 0;
			CopyFile("ScreenTestGameCode.dll", "ScreenTestGameCode_temp.dll", FALSE);
			GameCodeDLL = LoadLibraryA("ScreenTestGameCode_temp.dll");
			if(GameCodeDLL){
				// Re-point that function pointer to the function in our DLL
				GameUpdateAndRender = (game_update_and_render *)GetProcAddress(GameCodeDLL, "GameUpdateAndRender");
				GameUpdateSound = (game_update_sound *)GetProcAddress(GameCodeDLL, "GameUpdateSound");
			}
			
			uint32 LoadCounter = 0;
			
			game_state GameState = {};
			
			
			// This will be used to give our controller information to the GameCode
			game_controller_input Controller = {};
			
			// This will be used to give our GameCode access to the soundcard
			game_sound_output_buffer GameSoundBuffer = {};
			GameSoundBuffer.SamplesPerSecond = SamplesPerSecond;
			GameSoundBuffer.SampleCount = 0;//RunningSampleIndex;
			bool GameCodeIsValid;
			
			// Store the Last time the game code dll was written
			FILETIME LastDLLWriteTime = {};
			WIN32_FILE_ATTRIBUTE_DATA GameDLLData;
			
			// Get the Timestamp of the time that the ScreenTestGameCode.dll was changed
			if(GetFileAttributesExA("ScreenTestGameCode.dll", GetFileExInfoStandard, &GameDLLData)) {
				LastDLLWriteTime = GameDLLData.ftLastWriteTime;
			}
			
			int GameCodeLoadBuffer = 0;	
			
			// START THE PROGRAM LOOP
			// ------------------------------
			// ------------------------------
			// ------------------------------
			// ------------------------------
			// ------------------------------
            GlobalRunning = true;
			while(GlobalRunning){
				// RELOAD GAMECODE IF IT CHANGES
				// ---------------
				// ---------------
				// Get the New Timestamp of the time that the ScreenTestGameCode.dll was changed
				FILETIME NewDLLWriteTime = {};
				if(GetFileAttributesExA("ScreenTestGameCode.dll", GetFileExInfoStandard, &GameDLLData)) {
					NewDLLWriteTime = GameDLLData.ftLastWriteTime;
				}
				
				// Compare said New Timestamp with the Old Timestamp (from the last frame)
				// If it's different, we will reload the Game Code Dll
				if(CompareFileTime(&NewDLLWriteTime, &LastDLLWriteTime)!=0) {
					// First Unload the Game Code Dll, which will free up our temp Dll file
					if(GameCodeDLL){
						FreeLibrary(GameCodeDLL);
						GameCodeDLL = 0;
					}
					GameCodeIsValid = false;
					GameUpdateAndRender = 0;
					
						// Then copy the new gamecode to the freed up temp dll file
						CopyFile("ScreenTestGameCode.dll", "ScreenTestGameCode_temp.dll", FALSE);
						// Load that temp dll file
						GameCodeDLL = LoadLibraryA("ScreenTestGameCode_temp.dll");
						// If the load is successful, point our GameUpdateAndRender function pointer to the GameUpdateAndRender
						// function in the new dll file
						if(GameCodeDLL) {
							GameUpdateAndRender = (game_update_and_render *)GetProcAddress(GameCodeDLL, "GameUpdateAndRender");
							GameCodeIsValid = (GameUpdateAndRender);
						}
						// If the load is unsuccessfull, point our GameUpdateAndRender function pointer to our fake function 
						if(!GameCodeIsValid) {		
							GameUpdateAndRender = &GameUpdateAndRenderStub;
						}
					if(++GameCodeLoadBuffer>1) {
						// For some reason loading the file on the frame that it gets unloaded will cause Windows to complain
						// with an error, so I am using this variable to make it so it will actually load the new dll file
						// on the next frame after this
						GameCodeLoadBuffer=0;
					}
				}
				LastDLLWriteTime=NewDLLWriteTime;
				
					
				// HANDLE WINDOWS MESSAGES
				// ---------------
				// ---------------
				// Allocate enough memory on the stack for a MSG structure, so we can place the stuff GetMessage()
                // spits out into it
                 MSG Message;

                /* The PeekMessage() function will reach into the innards of the window handle we just
                // created and grab whatever message (eg window resize, window close, etc..) is queued
                // up next. It will then send the raw data of that message to the memory we allocated
                // for the MSG structure above*/
                while(PeekMessage(&Message,0,0,0,PM_REMOVE)){
                    if(Message.message == WM_QUIT){
                        GlobalRunning = false;
                    }

					// FUN KEYBOARD STUFF
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
									GlobalRunning = false;
								} else if (VKCode == VK_SPACE) {
								} else if (VKCode == 'L') {
								}
								bool AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
								if ((VKCode == VK_F4) && AltKeyWasDown){
									GlobalRunning = false;
									PostQuitMessage(0);
								}
							}

						} break;
						
						default: {
								// If we don't care about the message, we'll just send it to the Windows CALLBACK function, and let
								// it handle it
								TranslateMessage(&Message);
								DispatchMessage(&Message);
						} break;
					}
                }
				
				// HANDLE JOYPAD INPUT
				// ---------------
				// ---------------	
				XINPUT_STATE ControllerState;
				if(XInputGetState(0, &ControllerState) == ERROR_SUCCESS){ 
					Controller.IsConnected = true;
					XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
					
					// Map the buttons from Xinput to our controller	
					// DPad Buttons
					Controller.MoveUp = (bool)(Pad->wButtons&XINPUT_GAMEPAD_DPAD_UP);
					Controller.MoveDown = (bool)(Pad->wButtons&XINPUT_GAMEPAD_DPAD_DOWN);
					Controller.MoveLeft = (bool)(Pad->wButtons&XINPUT_GAMEPAD_DPAD_LEFT);
					Controller.MoveRight = (bool)(Pad->wButtons&XINPUT_GAMEPAD_DPAD_RIGHT);
					
					// Action buttons
					Controller.ActionDown = (bool)(Pad->wButtons&XINPUT_GAMEPAD_A);
					Controller.ActionRight = (bool)(Pad->wButtons&XINPUT_GAMEPAD_B);
					Controller.ActionLeft = (bool)(Pad->wButtons&XINPUT_GAMEPAD_X);
					Controller.ActionUp = (bool)(Pad->wButtons&XINPUT_GAMEPAD_Y);
					Controller.LeftShoulder = (bool)(Pad->wButtons&XINPUT_GAMEPAD_LEFT_SHOULDER);
					Controller.RightShoulder = (bool)(Pad->wButtons&XINPUT_GAMEPAD_RIGHT_SHOULDER);
					
					// Menu buttons
					Controller.Start = (bool)(Pad->wButtons&XINPUT_GAMEPAD_START);
					Controller.Back = (bool)(Pad->wButtons&XINPUT_GAMEPAD_BACK);
					
					if(Pad->wButtons&XINPUT_GAMEPAD_BACK) {
						GlobalRunning = false;
					}
					
					
					// Left Thumbstick
					Controller.IsAnalog = true;
					real32 StickAverageX = 0;
					if(Pad->sThumbLX < - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2){
						StickAverageX = (real32)((Pad->sThumbLX + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2) / (32768.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2));							
					} else if (Pad->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2) {
						StickAverageX = (real32)((Pad->sThumbLX + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2) / (32767.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2));
					}
					Controller.StickAverageX = StickAverageX;
					real32 StickAverageY = 0;
					if(Pad->sThumbLY < - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE){
						StickAverageY = (real32)((Pad->sThumbLY + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2) / (32768.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2));							
					} else if (Pad->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2) {
						StickAverageY = (real32)((Pad->sThumbLY + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2) / (32767.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/2));
					}
					Controller.StickAverageY = StickAverageY;
					
					XOffset += (int)(8.0f*(StickAverageX));
					YOffset -= (int)(8.0f*(StickAverageY));
					ToneHz = 256 + (int)(128.0f*(StickAverageY));
				} else {
					Controller.IsConnected = false;
				}
				
				// MAKE SOUND HAPPEN
				// ---------------
				// ---------------
				
				// We'll need this for timing stuff
				LARGE_INTEGER AudioWallClock = Win32GetWallClock();
				double FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);
				
				// Allocate space on the stack for the PlayCursor 
				DWORD PlayCursor;
				
				// Allocate space on the stack for the WriteCursor
				DWORD WriteCursor;
				
				// GetCurrentPosition will give you the current position of the PlayCursor and WriteCursor
				if(GlobalSoundBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor) == DS_OK){				
					// HOW THE SOUND WILL BE ADDED TO THE SOUND BUFFER
					{
					/* 
					   In order to write data into the SoundBuffer without messing up the audio, we will have
					   to plan exactly where we are going to START writing and where we are going to END writing
					   each time we go through our game loop.
					   
					   We must not START writing anytime before the WriteCursor's position. And we must not END
					   writing anytime after the PlayCursor position.
					  
					   PART 1 - GETTING OUR START POSITION
					   We will always START writing at the RunningSampleIndex value. The FIRST time through, this 
					   value will be set to the exact position of the WriteCursor:
					  
									  RunningSampleIndex
								   start here v
					   [---[-]---------------{-}----------------]
					    PlayCursor       WriteCurosr
						
					   as we write data (DDDD) into the SoundBuffer, the RunningSampleIndex will move 
					   forward:
					   
									  ---------> RunningSampleIndex
											             v end here 
					   [---[-]---------------{-}DDDDDDDDD-------]
					    PlayCursor       WriteCurosr
					   
					   So the NEXT time through, when we start at the RunningSampleIndex, we'll start exactly
					   where we left off previously:
					   
												RunningSampleIndex
									          start here v
					   [---[-]---------------{-}DDDDDDDDD-------]
					    PlayCursor       WriteCurosr
					   
					   And we'll just do that for every other time through.
					   
					   
					   PART 2 - CALCULATING THE TARGET CURSOR
						Our TargetCursor is where we plan to END writing our data, and subsequently, 
						it's where we plan to START writing our data at the next loop. We want our sound 
						to have as low a latency as possible, so we have to be careful when we're deciding 
						where we want the TargetCursor to be. 
						
						It's actually better to to think of this in terms of frames than just samples since we
						want to have our audio match our video as close as possible.
						
						When we're thinking of just sound samples, we can picture a space in memory that loops 
						around itself forever:
						
						[------[-]--------{-}----]
						 --->PCursor--->WCursor--
						 
						When we're thinking in frames, however, it's better to think of a space in memory that
						goes forward infinitely and has boundaries for each frame:
						 
							   frame 2                        frame 3                    frame 4
						   
						[----------[-]-------{-}-----|--------------------------|--------------------------]
						   ----->PCursor-->WCursor
						
						With this in mind, let's get to how we're going to calculate the TargetCursor.
						
						We define a safety value (SafetyBytes) that is the number of samples we think our game 
						update loop may vary by (let's say up to 2ms)
						
						We will look to see where the PlayCurosr is:						
							   frame 2                        frame 3                    frame 4
						   
						[----[-]---------------------|--------------------------|--------------------------]
							  ^
						   PCursor << this position
						
						
						and we will forcast ahead where we think the play cursor will be on the next frame
						boundary:							 
								frame 2                        frame 3                    frame 4	
												   THIS SPOT						  
													   v
						[----[-]---------------------|[*]----------------------|--------------------------]
							  ^                        ^
							PCursor------------->futurePCursor << this position     
													   
												   
						We will then look to see if the write cursor (WC) is BEFORE that by at least our safety 
						value (s):
								frame 2                        frame 3                    frame 4	   
							  (WC is BEFORE..) (..FrameBoundary - ssss)								
											v   v
						[----[-]-----------{-}--ssss|[ ]-----------------------|--------------------------]
							  ^                       ^
							PCursor------------->futurePCursor     
						
						
						If it is, the target fill position (X) is that frame boundary plus one frame.
						This gives us perfect audio sync in the case of a card that has low enough latency: 
								frame 2                        frame 3                    frame 4
																		TargetFillPosition						  
										 WC v           						v
						[----[-]-----------{d}ddddddd|[d]dddddddddddddddddddddd|X-------------------------]
							  ^                        ^
							PCursor------------->futurePCursor
				
						
						If the write cursor is AFTER that safety margin (s): 
						
								frame 2                        frame 3                    frame 4	    
								   (..frame boundary - ssss)  (WC is after..)
												 v              v
						[----[-]-----------------ssss|[ ]------{-}--------------|-------------------------]
							  ^                        ^
							PCursor------------->futurePCursor     
						
						
						Then we assume that we can never sync the audio perfectly, so we will write one 
						frame's worth of audio (d) plus the safety margin's worth of guard samples (s):
						
								frame 2                        frame 3                    frame 4	    
																							  TargetFillPosition
															 WC v									   v
						[(-)-[-]---------------------|[ ]------{-}ssssdddddddddd|ddddddddddddddddddddddX--]
							  ^                        ^			 [---------------------------------]
							PCursor------------->futurePCursor	 [----]		+	one frame of audio
															 saftey margin
						
						And that is our plan.
					*/										 
					
					// So let's get this thing working
					}
					
					// If first time through the loop
					if(!SoundIsValid) {
						// Get the offset address in the buffer where the sound currently is playing
						RunningSampleIndex = WriteCursor / BytesPerSample;
						// Never first time again
						SoundIsValid = true;
					}
					
					// ByteToLock represents where we will START when we lock data in the SoundBuffer.
					DWORD ByteToLock = 0;
					 
					DWORD ExpectedSoundBytesPerFrame = (SamplesPerSecond * BytesPerSample) / GameUpdateHz;
					
					
					double SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);					
					DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame)*(real32)ExpectedSoundBytesPerFrame);
					DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;
					
					// SafeWriteCursor = our WriteCursor + SafetyBytes
					DWORD SafeWriteCursor = WriteCursor;
					if(SafeWriteCursor < PlayCursor) {
						SafeWriteCursor += SoundBufferSize;
					} 
					//Assert(SafeWriteCursor >= PlayCursor);
					SafeWriteCursor += SafetyBytes;
					
					bool32 AudioCardIsLowLatency = SafeWriteCursor < ExpectedFrameBoundaryByte;
					
					// TargetCursor represents where we will END when we lock data in the SoundBuffer
					DWORD TargetCursor = 0;
					if(AudioCardIsLowLatency) {
						TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
					} else {
						TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + (SafetyBytes));
					}
					TargetCursor = TargetCursor % SoundBufferSize;
					
					ByteToLock = (RunningSampleIndex*BytesPerSample) % SoundBufferSize;
					
					// BytesToWrite is the variable we will use to calculate how we long we will move 
					// until we STOP in the SoundBuffer, so it will be calculated using the TargetCursor.
					DWORD BytesToWrite = 0;
					if(ByteToLock > TargetCursor){
						BytesToWrite = (SoundBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					} else {
						BytesToWrite = TargetCursor - ByteToLock;
					}
					
					// UPDATE THE GAME
					// ---------------
					// ---------------
					// Get the Video Data from the game
					game_offscreen_buffer GameBuffer = {};
					GameBuffer.Memory = PointerToBackBuffer->Memory;
					GameBuffer.Width = PointerToBackBuffer->Width;
					GameBuffer.Height = PointerToBackBuffer->Height;
					GameBuffer.Pitch = PointerToBackBuffer->Pitch;
					// Get the Sound Data from the game
					GameSoundBuffer.Samples = Samples;
					GameSoundBuffer.SampleCount = BytesToWrite / BytesPerSample;
					if(GameUpdateAndRender != 0) {
						GameUpdateAndRender(&GameBuffer, &GameState, &Controller, &GameSoundBuffer);						
					}
					
					/* FILL THE SOUND BUFFER WITH OUR GENERATED SOUND
					// This is the point where we FINALLY write our sound data to the actual sound card,
					// (albeit indirectly through Windows's fake Sound Card memory)
					
					// We start by shoving those previous Region pointers into the Lock method, which will point 
					// them to the fake "sound card" area of memory Windows gives us to write into*/
					if(SUCCEEDED(GlobalSoundBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size,&Region2, &Region2Size,0 ))) {	
						/* Then we loop through the memory of the sound card area, while simultaneously looping
						// through the Samples memory of our generated sound, and fill in each individual 16 bit
						// piece of data, one piece at a time:
						
						// First we do it for region1*/
						DWORD Region1SampleCount = Region1Size/BytesPerSample;
						int16 *DestSample = (int16 *)Region1;
						int16 *SourceSample = Samples;
						for(DWORD SampleIndex=0; SampleIndex < Region1SampleCount; ++SampleIndex) {
							*DestSample++ = *SourceSample++; // stereo sound left
							*DestSample++ = *SourceSample++; // stereo sound right 
							++RunningSampleIndex;
						}
						// Then we do it for region2
						DWORD Region2SampleCount = Region2Size/BytesPerSample;
						DestSample = (int16 *)Region2;
						for(DWORD SampleIndex=0; SampleIndex < Region2SampleCount; ++SampleIndex){
							*DestSample++ = *SourceSample++; // stereo sound left
							*DestSample++ = *SourceSample++; // stereo sound right
							++RunningSampleIndex;
						}
						
						// The final thing we have to do is Unlock the data we Locked. If we don't do this,
						// the sound will get messed up right before we start writing again.
						GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
					}	
							
				} else {
					SoundIsValid = false;
				}

				
				
				LARGE_INTEGER WorkCounter = Win32GetWallClock();
				double SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, WorkCounter);
				
				
				// LIMIT THE FPS (TODO: explain)
				// ---------------
				// ---------------
				if(SecondsElapsedForFrame < TargetSecondsPerFrame) {
					if(SleepIsGranular) {
						DWORD SleepMS = (DWORD)(800.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
						if(SleepMS > 0){
							Sleep(SleepMS);								
						}
					}
					#if 0
					#endif
					//std::cout << "\nO:";
					while(SecondsElapsedForFrame < TargetSecondsPerFrame) {
						//std::cout << "x";
						SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
					}
				LARGE_INTEGER TestCounter = Win32GetWallClock();
				double SecondsElapsedSinceSleep = Win32GetSecondsElapsed(WorkCounter, TestCounter);					
				//std::cout << SecondsElapsedSinceSleep << "\n\n";					
				} else {
					std::cout << "missed frame\n";
					// Missed frame rate logging
				}
				
				// FPS Tracking
				real32 SecondsPerFrame = Win32GetSecondsElapsed(LastCounter,Win32GetWallClock());
				double FramesPerSecond = 1.0d/ SecondsPerFrame;
				//std::cout << "FPS: " << FramesPerSecond << "\n";
				LARGE_INTEGER EndCounter = Win32GetWallClock();
				LastCounter = EndCounter;
				
				// DISPLAY BUFFER IN WINDOW
				// ---------------
				// ---------------
				// RECT is the data format that Windows uses to represent the dimensions of
				// a window (HWND) that has been opened.
				// v This command v allocates the amount of space needed for that RECT on the stack
				RECT ClientRect;

				// v This v is a windows.h command that will get the RECT of any given window
				// However, instead of RETURNING the value like a normal function would do, this function shoves the value into an already allocated space of memory
				// (in our case this space of memory would be the one that was created by our ^ RECT ClientRect; ^ command above)
				GetClientRect(Window, &ClientRect);

				// Now we're getting the width and height (which is all we want) from that RECT
				WindowWidth = ClientRect.right - ClientRect.left;
				WindowHeight = ClientRect.bottom - ClientRect.top;
				
				// StretchDIBits takes the color data (what colors the individual pixels are) of an image
				// And puts it into a destination
				StretchDIBits(DeviceContext,
								0, 0, PointerToBackBuffer->Width, PointerToBackBuffer->Height,
								0, 0, PointerToBackBuffer->Width, PointerToBackBuffer->Height,
								PointerToBackBuffer->Memory,
								&PointerToBackBuffer->Info,
								DIB_RGB_COLORS,
								SRCCOPY
								);
				// It is said that if you Get a DC (Device Context), you must release it when you're done with it. I have no idea why. 
				// Probably a memory allocation thing.
				// Commenting this out doesn't seem to negatively effect the program, so I assume that since this program only gets the DC once, it will
				// be forced to release that DC when you exit the program
				// ReleaseDC(Window, DeviceContext);
				

            }
			::ShowWindow(::GetConsoleWindow(), SW_SHOW);
        } else {
            // TODO(Casey): Logging
        }
    } else {
        // TODO(Casey): Logging
    }

    return(0);
}