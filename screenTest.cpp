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
#include <limits>

// This will allow you to make windows create a window for you
#include <windows.h>

// This will allow you to use Microsoft's direct sound definitions
#include "mingwdsound.h"

// This will allow you to use Microsoft's Xinput definitions
#include "mingwxinput.h"
//#include <dsound.h>

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

// NOTE(Casey): XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
	return(ERROR_DEVICE_NOT_CONNECTED);
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(Casey): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
	return(ERROR_DEVICE_NOT_CONNECTED);
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// This is totally hacky way to be able to use the direct sound DLL without having to use the direct sound library
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuideDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// ( v This is NOT the start of the program. v It is a function that windows REQUIRES if you
// want to open a window. When you create a window class (WNDCLASSA), one of its properties
// will be a reference to this function )
// This function is here so it can be called when something happens to the window
// If something happens to a window, it will send a Message to this callback
// eg. messages = Window resized (WM_SIZE),window clicked on (WM_ACTIVATEAPP), etc.
// LRESULT CALLBACK is a Long Pointer that points to the start of this function
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
							0, 0, Width, Height,
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
    // HINSTANCE - Instance handle of the application. Windows is set up to allocate us
    // our own virtual memory space. The HINSTANCE is the way we tell Windows that we
    // want to access addresses within that virtual memory
    HINSTANCE Instance,
    HINSTANCE PrevInstance, // null
    LPSTR CmdLine, // if any parameters are put in command line when the program is run
    int ShowCode // determines how app window will be displayed
) {
	// Hide the console (Windows XP shows the console even though this is a WinMain function when it's compiled with MinGW)
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
	
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
	int ScreenWidth = 1280;
	int ScreenHeight = 720;
	int BytesPerPixel = 4;
    PointerToBackBuffer->Width = ScreenWidth;
    PointerToBackBuffer->Height = ScreenHeight;  
    PointerToBackBuffer->Pitch = ScreenWidth*BytesPerPixel;

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
	// -v-----------v-
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
            CW_USEDEFAULT,                  //nWidth
            CW_USEDEFAULT,                  //nHeight
            0,                              //hWndParent - 0 if only one window
            0,                              //hMenu 0 - if not using any windows menus
            Instance,                       //hInstance - the handle to the app instance
            0                               //lpParam
        );
	// ---------------
	// -^-----------^-
	// END OF OPEN A WINDOWS WINDOW
		
		// If we succed at opening the Windows window, we will start initializing all our
		// game stuff
        if(Window){
			// This is the framerate we will try to hit
			int GameUpdateHz = 30;			

			// INITIALZE SOUND
			// ---------------
			// ---------------
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
			// There are several ways you can aske Windows for access to this virtual sound
			// buffer, but the easiest is through an old Windows Library called DirectSound. We are
			// going to use that library.
			
			// Instead of importing the DirectSound Lib file and linking it, which would be annoying 
			// in MinGW, we will just import the DLL. LoadLibraryA will tell Windows to search 
			// for the DLL in its inner libraries, I believe, but don't quote me on that.
			HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
			
			// The speed at which this wave is read is called the SamplesPerSecond, it's also known as 
			// the hz (48000hz, 44100hz, 22000hz, etc)
			// The higher the SamplesPerSecond, the more data you can fit into a second in sound, and 
			// the higher quality your sound will be.
			// We'll set our sound's SamplesPerSecond to 48000hz, since that's the standard for high
			// quality audio
			int SamplesPerSecond = 48000;
			
			// The RunningSampleIndex is a variable we're creating to represent the individual sample 
			// that the sound card is currently on in our data when it's generating the wave 
			uint32 RunningSampleIndex = 0; 
			
			// BytesPerSample. Each sample is a particular height at a particular time. That height
			// will represent the volume of that sound at that time. The BytesPerSample is how 
			// granularly you can set that height. We are going to set it to 2 bytes, which is the 
			// size of an int16. Since our sound is going to be stereo, we will need it to be the size
			// of 2 int16 vars.
			int BytesPerSample = sizeof(int16)*2;
			
			// SoundBufferSize = how big we want our SoundBuffer to be. We are going to make it as big as 
			// one second of sound.
			DWORD SoundBufferSize = SamplesPerSecond*BytesPerSample;
			
			// This is the amount we will allow the sound to fall behind the video to make sure
			// that our sound gets written before the sound card plays it. If it gets written 
			// at the same time or after the sound card plays it, we'll get scratchy, glitchy 
			// audio. We are measuring this time in frames because it's easier to think about.
			int FramesOfAudioLatency = 3;
			
			// LatencySampleCount is the amount we will let the sound fall behind the video in order to be
			// able to write it correctly.
			int LatencySampleCount = FramesOfAudioLatency*(SamplesPerSecond / GameUpdateHz);
			
			// We will only initialize DirectSound if windows is able to find the library.
			if(DSoundLibrary){
				// Get a DirectSound object - this is a hacky way to do it since we're not actually using the library but the DLL
				direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
				
				// For Direct Sound, we need to initialize a Primary Buffer and a Secondary Buffer.
				// The Primary Buffer used to be a way to write data directly into to the sound card itself, 
				// but at some point, it was decided that writing directly into hardware wasn't something
				// that should be done anymore, so now the Primary Buffer is just used to set up the format
				// of the sound. 
				// The Secondary Buffer will go to the fake address Windows creates for you to virtually access
				// the sound card. When you put data into the Secondary Buffer, Windows will copy that data 
				// into the sound card for you.
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
										// The format has successfully been set
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
			
			// The first thing we need to do is create those pointers.
			VOID *Region1;
			DWORD Region1Size;
			VOID *Region2;
			DWORD Region2Size;
			
			/*
			REGION1 and REGION2
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
			
			
			// Alright, let's shove these pointers into the Lock method, and get our
			// SoundBuffer address, so we can clear the whole thing
			if(SUCCEEDED(GlobalSoundBuffer->Lock(0, SoundBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0 ))) {
				// Since we're basically grabbing all of the data from 0, this will,
				// in theory, only have to use *Region1, but we're going to loop
				// through both regions just in case
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
			
			// ALLOCATE PRE-WRITE SOUND GENERATION MEMORY FOR SOUND BUFFER
			// This is the memory we are going to use to generate some of our own sound before 
			// it gets written into the SoundBuffer memory
			int16 *Samples = (int16 *)VirtualAlloc(0, SoundBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			
			DWORD LastPlayCursor = 0;
			// We will keep the sound off until it's lined up correctly
			bool SoundIsValid = false;
	
	
			// INITIALIZE VIDEO
			// ---------------
			// ---------------
            // The Device Context is the place in memory where the drawing will go.
			// v This function v will temporarily grab the Device Context of the Window we've just created, so we can draw into it
            HDC DeviceContext = GetDC(Window);

			// Set up variables to make the weird gradient move
			int XOffset = 0;
            int YOffset = 0;
			int XSpeed = 0;
			int YSpeed = 0;
			int Speed = 2;
			int ToneHz = 256;
			
			
			// START THE PROGRAM LOOP
            GlobalRunning = true;
			while(GlobalRunning){
				// HANDLE WINDOWS MESSAGES
				// ---------------
				// ---------------
				// Allocate enough memory on the stack for a MSG structure, so we can place the stuff GetMessage()
                // spits out into it
                 MSG Message;

                // The PeekMessage() function will reach into the inards of the window handle we just
                // created and grab whatever message (eg window resize, window close, etc..) is queued
                // up next. It will then send the raw data of that message to the memory we allocated
                // for the MSG structure above
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
					XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
					
					// DPad Buttons
					if(Pad->wButtons&XINPUT_GAMEPAD_DPAD_UP) {
						YOffset=-Speed;
					}
					if(Pad->wButtons&XINPUT_GAMEPAD_DPAD_DOWN) {
						YOffset=+Speed;
					}
					if(Pad->wButtons&XINPUT_GAMEPAD_DPAD_LEFT) {
						XOffset=-Speed;
					}
					if(Pad->wButtons&XINPUT_GAMEPAD_DPAD_RIGHT) {
						XOffset=+Speed;
					}
					
					// Action buttons
					if(Pad->wButtons&XINPUT_GAMEPAD_A) {
						
					}
					if(Pad->wButtons&XINPUT_GAMEPAD_B) {
						
					}
					if(Pad->wButtons&XINPUT_GAMEPAD_X){
						
					}
					if(Pad->wButtons&XINPUT_GAMEPAD_Y) {
						
					}
					if(Pad->wButtons&XINPUT_GAMEPAD_RIGHT_SHOULDER) {
						
					}
					if(Pad->wButtons&XINPUT_GAMEPAD_LEFT_SHOULDER) {
						
					}
					if(Pad->wButtons&XINPUT_GAMEPAD_START) {
						
					}
					if(Pad->wButtons&XINPUT_GAMEPAD_BACK) {
						GlobalRunning = false;
					}
					
					
					// Left Thumbstick
					real32 StickAverageX = 0;
					if(Pad->sThumbLX < - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE){
						StickAverageX = (real32)((Pad->sThumbLX + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32768.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));							
					} else if (Pad->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
						StickAverageX = (real32)((Pad->sThumbLX + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32767.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
					}
					real32 StickAverageY = 0;
					if(Pad->sThumbLY < - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE){
						StickAverageY = (real32)((Pad->sThumbLY + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32768.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));							
					} else if (Pad->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
						StickAverageY = (real32)((Pad->sThumbLY + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32767.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
					}
					
					XOffset += (int)(4.0f*(StickAverageX));
					YOffset -= (int)(4.0f*(StickAverageY));
					ToneHz = 256 + (int)(128.0f*(StickAverageY));
				}
				
				// MAKE THE SOUND HAPPEN
				// ---------------
				// ---------------
				// Before we can write sound data into the SoundBuffer, we need to calculate
				// exactly WHERE we want to write  it and HOW MUCH of it we want to write.
				// Let's do that.
				
				// ByteToLock is the variable we will use to calculate where we will START when 
				// we lock data in our SoundBuffer.
				DWORD ByteToLock = 0;
				
				// TargetCursor is the variable we will use that will point to the address where 
				// we will want to END in the SoundBuffer.
				DWORD TargetCursor = 0;
				
				// BytesToWrite is the variable we will use to calculate how we long we will move 
				// until we STOP in the SoundBuffer, so it will be calculated using the TargetCursor.
				DWORD BytesToWrite = 0;
				
				
				/* 
				Ideally we want to Lock (L) our data starting (ByteToLock) at the 
				WriteCursor address and ending (BytesToWrite) just before the PlayCursor 
				address like so:
				  end here					  start here
				|LLLLL[*]-------------------------{*}LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL|
			      (PlayCursor)                (WriteCursor)

				We're not exactly going to do that.
				*/
				
				if(SoundIsValid){
					/* The FIRST time thru, RunningSampleIndex will be set to the 
					   WriteCursor address, so the data that our SoundBuffer Locks (L) will 
					   start (ByteToLock) at the WriteCursor address and end (BytesToWrite) 
					   at the place the PlayCursor address was at the start of our game loop (LastPlayCursor), 
					   which means we'll first Lock our sound from the WriteCursor address
					   like so:
					   
					     end here					start here
					  |LLLLL[*]-------------------------{*}LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL|
						 (PlayCursor)               (WriteCursor)
					  
					  Time will pass, and we will write our sound data (D) into the Locked area (L) 
					  as the PlayCursor moves toward it like so:            
									                  (look at all that beautiful sound data being written) 
																			   v 		
					  |LLLLL-------------------------[*]DDDDDDDDDDDDDDDDDDDDDDDDLLLLLL{L}LLLLL|
					          ------------------>(PlayCursor)   ----------------->(WriteCursor)
													  ^
				      (look at that evil PlayCursor Moving toward our precious data
				       good thing we already wrote it. check and mate, PlayCursor!)
					  
					  
					  Eventually, we'll have written all the data we Locked, like so (the 
					  PlayCursor will have started hitting the sound at this point, so we 
					  would probably hear the sound at this point):
					  
		(look at all that data we finshed writing)
					       v
					  |DDDDD--------------------------DDDDDD[D]DDDDDDDDDDDDDDDDDDDDD{D}DDDDDDD|
					                          --------->(PlayCursor)   -------->(WriteCursor)
															 ^
						             (look at that PlayCursor Playing our sound)
									 
					  And that's the end of our FIRST time thru. Wasn't it fun?
					  
					  
					  The SECOND time thru, RunningSampleIndex will already be set to the address directly 
					  after the last place we wrote our data like so:
					        
			       (RunningSampleIndex)
					        v
					  |DDDDD--------------------------DDDDDD[D]DDDDDDDDDDDDDDDDDDDDD{D}DDDDDDD|
					                          --------->(PlayCursor)   -------->(WriteCursor)

					  So we will Lock our data (L) starting at that position and ending at the place our
					  our PlayCursor was in the previous loop (LastPlayCursor).
				   (RunningSampleIndex)				 (LastPlayCursor)
							v						        v
					  |dddddLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL[ ]dd[d]ddddddddddddddddddddddd{d}d|
					                               --------->(PlayCursor)      ------>(WriteCursor)
						
					 Time will pass, and we will write our new sound data (D) as the PlayCursor continues 
					 to move toward it like the evil little PlayCursor it is:
					 
					      (look at all that brand new sound data being born for the first time!)
														 v
					 |dddddDDDDDDDDDDDDDD{D}DDDDDDDDDDDDDDddddddddddddddddddddddddddddd[d]dd|
					   ------------->(WriteCursor)                     --------->(PlayCursor)
																					  ^
													   (look at that wicked PlayCursor, getting ready to loop
														around and hit our precious new born data. Good thing we 
														already wrote it! You've been foiled again, PlayCursor!)
														
					 And that is the end of our SECOND time thru. After that, the process just repeats. We
					 Lock our SoundBuffer memory starting at the last place we left off (RunningSampleIndex)
					 and ending at the place the PlayCursor was in the last loop (LastPlayCursor), and we
					 fill it in with our sound data. We repeat these steps over and over ad infinitum until 
					 the program ends.
					 
					 And that, my friends, is how you spend way too long explaining how a SoundBuffer loop
					 works! I'm so hungry!
					*/
					
					// So let's get this thing working
					// Set the position we want to start Locking from in the SoundBuffer (ByteToLock) to 
					// the the place we were previously (RunningSampleIndex) (NOTE:this would be the 
					// WriteCursor position if this is the FIRST loop)
					ByteToLock = (RunningSampleIndex*BytesPerSample) % SoundBufferSize;
					
					// Instead of targeting our ending point exactly on the PlayCursor like would be ideal, 
					// we're going to (TODO: finish this explanation)
					TargetCursor = ((LastPlayCursor + (LatencySampleCount*BytesPerSample)) % SoundBufferSize);
					
					// If the spot we want to Lock in the SoundBuffer loops around, i.e. if it looks something
					// like this:
					//                      ---> end here              start here --->
					// |wherewewanttowritedata{TargetCursor}----------[ByteToLock]wherewewanttowritedata|
					if(ByteToLock > TargetCursor){
						// We first subtract the Starting Position (ByteToLock) from the total size of the 
						// SoundBuffer (SoundBufferSize):
						//                          1.  This length (SoundBufferSize) v
						// [--------------------------------------------------------------------------------]
						//           2. minus this length (ByteToLock) v 
						// [---------------------------------------------------------]
						//															3. equals this length v	
						//																  (BytesToWrite)
						//										1					 [----------------------]			
						// |wherewewanttowritedata{TargetCursor}----------[ByteToLock]wherewewanttowritedata|
						BytesToWrite = (SoundBufferSize - ByteToLock);
						
						// Then we add the length from the beginning of the buffer to the Ending Position 
						// (TargetCursor) to our value to get the final length we need:
						//         2. plus this length v                                 1. This length v
						//				(TargetCursor)								      (BytesToWrite)
						// [-----------------------------------]                     [----------------------]					
						//	-ngth (Final BytesToWrite) v								 3. equals this whole le-
						//	-----------------------------------]                     [----------------------
						// |wherewewanttowritedata{TargetCursor}----------[ByteToLock]wherewewanttowritedata|
						BytesToWrite += TargetCursor;
						
					// If the spot we want to Lock in the SoundBuffer DOESN'T loop around, i.e. it looks 
					// something like this:
					//       start here										  end here
					// |----[ByteToLock]wherewewanttowritedataaaaaaaaaaaaaaa{TargetCursor}--------------|
					} else {
						// We will subtract our starting position (ByteToLock) from our ending position
						// (TargetCursor) to get the length we need:
						//           1. This length v
						// [-----------------------------------------------------------------]
						//2. minus this v length
						// [---------------]
						//                      3. equals this v length (Final BytesToWrite)
						//                 [-------------------------------------------------]
						// |----[ByteToLock]wherewewanttowritedataaaaaaaaaaaaaaa{TargetCursor}--------------|
						BytesToWrite = TargetCursor - ByteToLock;
					}
				}
				
				// GENERATE OUR SOUND
				// As mentioned previously, we will use our own memory we allocated in the *Samples pointer
				// above.
				// For now, we will generate a simple sine wave.
				
				// The sample count will be set to the amount of data we plan to Lock in the SoundBuffer
				int SampleCount = BytesToWrite / BytesPerSample;
				
				//
				static real32 tSine=0;
				
				// This will be the volume we want our sine wave to hit.
				int16 ToneVolume = 3000;
				
				// This will be the length of a single section of sine wave, in order to make it resonate
				// at the exact tone we set ToneHz to, we have to divide it by the SamplesPerSecond
				int WavePeriod = SamplesPerSecond/ToneHz;
				
				// Set a pointer to the beginning of our Samples memory
				int16 *SampleOut = Samples;

				// Loop through our Samples memory and change each sample to the height it would be in 
				// our sine wave at the current time in the sound
				for(int SampleIndex=0; SampleIndex < SampleCount; ++SampleIndex) {
					// There is a formula we can use to generate where a sample would be in a Sine 
					// wave at a particular point in time. However, since we're starting with zero,
					// we'll just set it to the sine(0), to start off.
					// But since we're starting at 0, we can just grab the sign of zero, the first time
					real32 SineValue = sinf(tSine);
					int16 SampleValue = (int16)(SineValue * ToneVolume);
					
					// TODO: Fill this in later
					//  int16 int16   int16 int16   int16 int16  ...
					// [LEFT  RIGHT] [LEFT  RIGHT] [LEFT  RIGHT] ...
					*SampleOut++ = SampleValue;
					*SampleOut++ = SampleValue;
					// Here's the formula I mentioned above: 
					// Sin( ((2*PI) / WavePeriod) * CurrentTime )
					tSine += ((2.0f*Pi32) / (real32)WavePeriod)*1.0f;
					// Normalize the sine
					if(tSine > 2.0f*Pi32) {
						tSine -= 2.0f*Pi32;
					}
					// The next time around, we'll get the sine(tSine)
				}
				
				
				// FILL THE SOUND BUFFER WITH OUR GENERATED SOUND
				if(SoundIsValid){
					// This is the point where we FINALLY write our sound data to the actual sound card,
					// (albeit indirectly through Windows's fake Sound Card memory), i.e. we'll be doing this:
					/*
																(sound data being written) 
																			   v 		
					  |LLLLL-------------------------[*]DDDDDDDDDDDDDDDDDDDDDDDDLLLLLL{L}LLLLL|
					          ------------------>(PlayCursor)   ----------------->(WriteCursor)
													  ^
									(PlayCursor heading toward sound data)
					*/
					
					// We start by shoving those previous Region pointers into the Lock method, which will point 
					// them to the fake "sound card" area of memory Windows gives us to write into
					if(SUCCEEDED(GlobalSoundBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size,&Region2, &Region2Size,0 ))) {				  
						
						// Then we loop through the memory of the sound card area, while simultaneously looping
						// through the Samples memory of our generated sound, and fill in each individual 16 bit
						// piece of data, one piece at a time:
						
						// First we do it for region1
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
				}
				
				//  As the buffer is played, the play cursor moves and always points to the next byte of data to be played. 
				DWORD PlayCursor;
				
				// The write cursor is the point after which it is safe to write data into the buffer. The block between the play cursor 
				// and the write cursor is already committed to be played, and cannot be changed safely.
				DWORD WriteCursor;
				
				// GET CURRENT SOUND POSITION
				// In order to know what point in the SoundBuffer we want to write up to without messing with
				// the sound that is currently playing, we need to grab the 
				// the PlayCursor's current position and save it (LastPlayCursor)
				if(GlobalSoundBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor) == DS_OK){
					// Save the current position for the next time we run through the loop
					LastPlayCursor = PlayCursor;
					
					// This stuff will only happen if it's the first time through the loop. It will give us
					// a position to start writing sound from.
					if(!SoundIsValid) {
						// Get the offset address in the buffer where the sound currently is playing
						RunningSampleIndex = WriteCursor / BytesPerSample;
						// Turn the sound on
						SoundIsValid = true;
					}
				} else {
					SoundIsValid = false;
				}

				// RENDER A WEIRD GRADIENT THING
				// ---------------
				// ---------------
				uint8 *Row = (uint8*)GlobalBackBuffer.Memory;
				for(int Y = 0; Y<GlobalBackBuffer.Height; ++Y){
					uint32 *Pixel = (uint32 *)Row;
					for(int X = 0; X < GlobalBackBuffer.Width;
					++X) {
						/*
							Pixel in Memory: BB GG RR xx
							LITTLE ENDIAN ARCHITECTURE

							0x xxBBGGRR
						*/
					   uint8 Blue = (X + XOffset);
					   uint8 Green = (Y + YOffset);
					   uint8 Red = (X + XOffset);

					   /*
						Memory:     BB GG RR xx
						Register:   xx RR GG BB
					   */

						*Pixel++ = ((Red << 16) | (Green << 8) | Blue);
					}
					Row += GlobalBackBuffer.Pitch;
				}

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
								0, 0, WindowWidth, WindowHeight,
								0, 0, PointerToBackBuffer->Width, PointerToBackBuffer->Height,
								PointerToBackBuffer->Memory,
								&PointerToBackBuffer->Info,
								DIB_RGB_COLORS,
								SRCCOPY
								);

				// Move the weird gradient from Right to left
				XOffset+=XSpeed;
				YOffset+=YSpeed;

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