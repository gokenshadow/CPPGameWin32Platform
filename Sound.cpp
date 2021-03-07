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
#include <stdio.h>
#include <iostream>

// This will allow you to make windows create a window for you
#include <windows.h>

// If you are compiling this with VSTUDIO make sure the VSTUDIO flag is set to 1
#if VSTUDIO
#include <xinput.h>
#include <dsound.h>
#else
// This will allow you to use Microsoft's direct sound definitions
#include "mingwdsound.h"
#endif

static bool GlobalRunning;

// Sound Buffer
static LPDIRECTSOUNDBUFFER GlobalSoundBuffer;

static int64 GlobalPerfCountFrequency;

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
LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
    LRESULT Result = 0;

    // Depending on the event that happens we want to do different things
    switch(Message){
        case WM_SIZE:
        {
        } break;
        case WM_DESTROY:
        {
			GlobalSoundBuffer->Stop();
			// Freeze for a second to let the sound buffer clear itself
			// if we don't do this, the sound will sound choppy for a split
			// second the next time we start the program
			Sleep(50);
            GlobalRunning = false;
			PostQuitMessage(0);
        } break;
		case WM_ACTIVATE:
        {
			
        } break;
        case WM_CLOSE:
        {
			GlobalSoundBuffer->Stop();
			// Freeze for a second to let the sound buffer clear itself
			// if we don't do this, the sound will sound choppy for a split
			// second the next time we start the program
			Sleep(50);
            GlobalRunning = false;
            PostQuitMessage(0);
        } break;
		// WM_PAINT is the event that happens when the window starts to draw. It will rehappen every time
		// the user messes with the window (resizes it, moves it around, unminimizes it, etc)
        case WM_PAINT:{
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
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			
			//PatBlt(DeviceContext, X, Y, Width, Height, BLACKNESS);
			
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
    WindowClass.lpszClassName = "Sound";

    // regeister the Window class
    if(RegisterClassA(&WindowClass)){

        // create a window handle, this will create our very own window that we can use for whatever we want
        HWND Window = CreateWindowExA(
            0,                              //dwExstyle
            WindowClass.lpszClassName,      //lpClassName
            "Sound",                //lpWindowName - name of window
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
		
		// If we succed at opening the Windows window, we will start initializing all our
		// sound stuff
        if(Window){
			bool IsWindowsXPOrHigher = true;
			OSVERSIONINFO osvi;
			ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&osvi);
			printf("Windows version: %u.%u\n", osvi.dwMajorVersion, osvi.dwMinorVersion);
			if(osvi.dwMajorVersion>5) {
				IsWindowsXPOrHigher = true;
			} else {
				IsWindowsXPOrHigher = false;
			}
			int Framerate = 60;
			double TargetSecondsPerFrame = 1.0f / (double)Framerate;		

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
			
			// If Windows 98, lower quality sound 
			
			int SamplesPerSecond;
			if(IsWindowsXPOrHigher) {
				SamplesPerSecond = 48000;
			} else {
				SamplesPerSecond = 22050;				
			}
			std::cout << "SamplesPerSecond" << SamplesPerSecond << "\n";
			
			
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
			DWORD SoundBufferSize;
			if(IsWindowsXPOrHigher) {
				SoundBufferSize = SamplesPerSecond*BytesPerSample;
			} else {
				SoundBufferSize = SamplesPerSecond*BytesPerSample*2;
			}
			
			/* There is always some variability in the timing of the actual hardware we're writing to, so
			// we're going to add a small amount of bytes that will account for that variabliity and
			// prevent it from messing with our sound. We'll call these bytes the SafetyBytes.*/
			DWORD SafetyBytes = (SamplesPerSecond*BytesPerSample);
			
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
					//std::cout << "blah\n";
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
			
			/* REGION1 and REGION2 EXPLANATION 				
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
		
			double FramesPerSecond = 0;
			
			// SET UP TIMING
			// ---------------
			// ---------------
			// We will need a counter that will count the current time
			LARGE_INTEGER LastCounter = Win32GetWallClock();
			LARGE_INTEGER FlipWallClock = Win32GetWallClock();
			
			
			// START THE PROGRAM LOOP
			// ------------------------------
			// ------------------------------
			// ------------------------------
			// ------------------------------
			// ------------------------------
			bool SoundPlaying = false;
            GlobalRunning = true;
			int Win98WaitTime = 4000;
			float Volume = 1.0f;
			if(!IsWindowsXPOrHigher) {
				Volume = 0;
			}
			while(GlobalRunning){ 
				if(Win98WaitTime > 0) {
					Win98WaitTime--;
				} else {
					Volume = 1.0f;
				}
				if(SoundIsValid==true&&SoundPlaying==false) {
					SoundPlaying=true;					
				}
				
				// HANDLE WINDOWS MESSAGES
				// ---------------
				// ---------------
				// Allocated space for a MSG on the stack
                 MSG Message;

                /* The PeekMessage() function will reach into the innards of the window handle we just
                // created and grab whatever message (eg window resize, window close, etc..) is queued
                // up next. It will then send the raw data of that message to the memory we allocated
                // for the MSG structure above*/
                while(PeekMessage(&Message,0,0,0,PM_REMOVE)){
                    if(Message.message == WM_QUIT){
                        GlobalRunning = false;
                    }

					// KEYBOARD KEYS THAT ALLOW YOU TO QUIT WITH ESCAPE
					switch(Message.message) {
						case WM_SYSKEYDOWN:
						case WM_SYSKEYUP:
						case WM_KEYDOWN:
						case WM_KEYUP:{
							uint32 VKCode = (uint32)Message.wParam;
							bool WasDown = ((Message.lParam & (1 << 30)) != 0);
							bool IsDown = ((Message.lParam & (1<< 31)) == 0);
							if(WasDown != IsDown) {
								if (VKCode == VK_ESCAPE) {
									if(IsDown) {
										GlobalSoundBuffer->Stop();
										// Freeze for a second to let the sound buffer clear itself
										// if we don't do this, the sound will sound choppy for a split
										// second the next time we start the program
										Sleep(50);
										GlobalRunning = false;										
									}
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
					/* HOW THE SOUND WILL BE ADDED TO THE SOUND BUFFER
					
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
					
					// If first time through the loop
					if(!SoundIsValid) {
						// Get the offset address in the buffer where the sound currently is playing
						RunningSampleIndex = WriteCursor / BytesPerSample;
						// Never first time again
						SoundIsValid = true;
					}
					
					// ByteToLock represents where we will START when we lock data in the SoundBuffer.
					DWORD ByteToLock = 0;
					 
					DWORD ExpectedSoundBytesPerFrame = (SamplesPerSecond * BytesPerSample) / Framerate;
					
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
					
					//std::cout << "AudioCardIsLowLatency:" << AudioCardIsLowLatency << "\n\n";
					// TargetCursor represents where we will END when we lock data in the SoundBuffer
					DWORD TargetCursor = 0;
					if(AudioCardIsLowLatency) {
						TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
					} else {
						TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + (SafetyBytes));
					}
					TargetCursor = TargetCursor % SoundBufferSize;
					
					ByteToLock = (RunningSampleIndex*BytesPerSample) % SoundBufferSize;
					//std::cout << "ByteToLock:" << ByteToLock << "\n";
					
					// BytesToWrite is the variable we will use to calculate how we long we will move 
					// until we STOP in the SoundBuffer, so it will be calculated using the TargetCursor.
					DWORD BytesToWrite = 0;
					if(ByteToLock > TargetCursor){
						BytesToWrite = (SoundBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					} else {
						BytesToWrite = TargetCursor - ByteToLock;
					}
					
					// GENERATE THE SOUND
					// ---------------
					// ---------------
					// For this, we'll generate a sine wave
					int16 ToneVolume = (int)(float)3000.0f*Volume;
					std::cout << "Tonevolume" << ToneVolume << "\n";
					int ToneHz = 256;
					static float tSine = 0.0f;
					int WavePeriod = SamplesPerSecond/ToneHz;

					int16 *SampleOut = Samples;
					int SampleCount = BytesToWrite / BytesPerSample;

					for(int SampleIndex=0; SampleIndex < SampleCount; ++SampleIndex) {
						real32 SineValue = sinf(tSine);
						int16 SampleValue = (int16)(SineValue * ToneVolume);

						*SampleOut++ = SampleValue;
						*SampleOut++ = SampleValue;

						tSine += ((2.0f*Pi32) / (real32)WavePeriod)*1.0f;

						if(tSine > 2.0f*Pi32) {
							tSine -= 2.0f*Pi32;
						}
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
						
						int blah = ByteToLock << 8;
						blah = blah >> 8;
						
						//std::cout << "PlayCursor: " << PlayCursor << "\n";
						//std::cout << "ByteToLock: " << ByteToLock << "\n";
						//std::cout << "WriteCursor: " << WriteCursor << "\n";
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
						//std::cout << "r:" << RunningSampleIndex << "\n";
						
						// The final thing we have to do is Unlock the data we Locked. If we don't do this,
						// the sound will get messed up right before we start writing again.
						GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
					}	
							
				} else {
					SoundIsValid = false;
				}
				
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
				int X = Paint.rcPaint.left;
				int Y = Paint.rcPaint.top;
				int Width = ClientRect.right - ClientRect.left;
				int Height = ClientRect.bottom - ClientRect.top;
				
				//PatBlt(DeviceContext, X, Y, Width, Height, BLACKNESS);
				
				EndPaint(Window, &Paint);
				
				LARGE_INTEGER WorkCounter = Win32GetWallClock();
				double SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, WorkCounter);

            }
        } else {
            // TODO(Casey): Logging
        }
    } else {
        // TODO(Casey): Logging
    }

    return(0);
}