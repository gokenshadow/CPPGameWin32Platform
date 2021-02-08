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

// This will allow you to make windows create a window for you
#include <windows.h>

// This will allow you to use Microsoft's direct sound definitions
#include "mingwdsound.h"
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
		
        if(Window){
			int GameUpdateHz = 30;
			int FramesOfAudioLatency = 3;
			
			// Initialize Sound
			// ---------------
			// ---------------
			int SamplesPerSecond = 48000;	// 48000hz, 44000hz, 22000hz, whatever you want, really
			uint32 RunningSampleIndex = 0;
			int BytesPerSample = sizeof(int16)*2;
			DWORD SoundBufferSize = SamplesPerSecond*BytesPerSample;
			int LatencySampleCount = FramesOfAudioLatency*(SamplesPerSecond / GameUpdateHz);			
			
			// Instead of importing the Lib file and linking it, which would be annoying in MinGW, 
			// we will just import the DLL instead. LoadLibraryA will tell Windows to search for the
			// DLL in it's libraries, I believe, but don't quote me on that.
			HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
			
			if(DSoundLibrary){
				// Get a DirectSound object
				direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
				
				// For Direct Sound, we need to initialize a Primary Buffer and a Secondary Buffer.
				// The Primary Buffer used to be a way to write data directly into to the sound card itself, 
				// but at some point, it was decided that writing directly into hardware wasn't something
				// that should be done anymore, so now the Primary Buffer is just used to set up the format
				// of the sound. 
				// The Secondary Buffer is where you will need to fill your sound data to make the to make it
				// come out of the sound card
				LPDIRECTSOUND DirectSound;
				if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
					
					// Set up the format of the sound
					WAVEFORMATEX WaveFormat = {};
					WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
					WaveFormat.nChannels = 2;	// 2 channels = stereo sound
					WaveFormat.nSamplesPerSec = SamplesPerSecond;	// 48000, 44000, 22000, whatever you want, really
					WaveFormat.wBitsPerSample = 16; // This will be the quality of the sound. 16 will be 32 bits total because its stereo sound
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
			
			
			// Clear the sound buffer 
			// The GlobalSoundBuffer->Lock method will basically take the pointers you give it and point 
			// them to an area in memory where you can write data that will actually be played by the sound 
			// card. We are going to get that memory, and clear it all to 0.
			// First, we create the pointers we need
			VOID *Region1;
			DWORD Region1Size;
			VOID *Region2;
			DWORD Region2Size;
			
			// Then, we shove those pointers into the Lock method, which will point them to the
			// "sound card" area we can write to
			if(SUCCEEDED(GlobalSoundBuffer->Lock(0, SoundBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0 ))) {
				// Finally we clear all of the data by looping through the samples and setting them all to 0
				uint8 *DestSample = (uint8 *)Region1; 
				for(DWORD ByteIndex=0; ByteIndex < Region1Size; ++ByteIndex) {
					*DestSample++ = 0;
				}
				DestSample = (uint8 *)Region2;
				for(DWORD ByteIndex=0; ByteIndex < Region2Size; ++ByteIndex) {
					*DestSample++ = 0;
				}
				GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
			}			
			
			// Play the sound buffer
			GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
			
			// Allocate memory for sound buffer
			// This is the memory we are going to use to write our sound into before it gets written into the "sound card" area
			// memory
			int16 *Samples = (int16 *)VirtualAlloc(0, SoundBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			
			bool SoundIsPlaying = false;
			DWORD LastPlayCursor = 0;
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
				// Something to do with sound. TODO:(fill this in later)
				// Get the amount of bytes we need to write into the sound buffer
				DWORD ByteToLock = 0;
				DWORD TargetCursor = 0;
				DWORD BytesToWrite = 0;
				if(SoundIsValid){
					ByteToLock = (RunningSampleIndex*BytesPerSample) % SoundBufferSize;
					TargetCursor = ((LastPlayCursor + (LatencySampleCount*BytesPerSample)) % SoundBufferSize);
					if(ByteToLock > TargetCursor){
						BytesToWrite = (SoundBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					} else {
						BytesToWrite = TargetCursor - ByteToLock;
					}
				}
				
				
				// GENERATE OUR SOUND
				// For now it's going to be a simple Sine wave.
				// The sample count is the amount of samples we will need to set for the current chunk of sound
				int SampleCount = BytesToWrite / BytesPerSample;
				static real32 tSine;
				int16 ToneVolume = 3000;
				int WavePeriod = SamplesPerSecond/ToneHz;
				int16 *SampleOut = Samples;
				// Loop through our allocated sound memory and change each sample to the height it would be in our sine wave at the
				// current time in the sound
				for(int SampleIndex=0; SampleIndex < SampleCount; ++SampleIndex) {
					real32 SineValue = sinf(tSine);
					int16 SampleValue = (int16)(SineValue * ToneVolume);
					*SampleOut++ = SampleValue;
					*SampleOut++ = SampleValue;			
					tSine += 2.0f*Pi32*1.0f/ (real32)WavePeriod;
				}
				
				// FILL THE SOUND BUFFER WITH OUR GENERATED SOUND
				if(SoundIsValid){
					// The GlobalSoundBuffer->Lock method will basically take the pointers you give it and point 
					// them to an area in memory where you can write data that will actually be played by the sound 
					// card. We are going to get that memory, and write our sine wave into it. We are going to use 
					// the previous pointers we created (*Region1, Region1Size, *Region2, Region2Size) to do this.
					
					// We start by shoving those previous pointers into the Lock method, which will point them to the
					// "sound card" area we can write to
					if(SUCCEEDED(GlobalSoundBuffer->Lock(ByteToLock, BytesToWrite,&Region1, &Region1Size,&Region2, &Region2Size,0 ))) {				  
						DWORD Region1SampleCount = Region1Size/BytesPerSample;
						
						// Then we loop through the memory of the sound card area, one 16-bit sample at a time, and 
						// fill each 16 bit sample with the same sample at the same time from our own generated samples
						int16 *DestSample = (int16 *)Region1;
						int16 *SourceSample = Samples;
						for(DWORD SampleIndex=0; SampleIndex < Region1SampleCount; ++SampleIndex) {
							*DestSample++ = *SourceSample++;
							*DestSample++ = *SourceSample++;
							++RunningSampleIndex;
						}
						DWORD Region2SampleCount = Region2Size/BytesPerSample;
						DestSample = (int16 *)Region2;
						for(DWORD SampleIndex=0; SampleIndex < Region2SampleCount; ++SampleIndex){
							*DestSample++ = *SourceSample++;
							*DestSample++ = *SourceSample++;
							++RunningSampleIndex;
						}
						
						// I don't know exactly what GlobalSoundBuffer->Unlock does, but if you don't use it, your sound
						// will play and glitch slightly everytime it loops back around TODO:(fill this in later)
						GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
					}
				}
				
				
				// TODO: (Fill this in later)
				DWORD PlayCursor;
				DWORD WriteCursor;
				if(GlobalSoundBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor) == DS_OK){
					LastPlayCursor = PlayCursor;
					if(!SoundIsValid) {
						RunningSampleIndex = WriteCursor / BytesPerSample;
						SoundIsValid = true;							
					}
				} else {
					SoundIsValid = false;
				}

				// RENDER A WEIRD GRADIENT THING
				// ---------------
				// ---------------
				// Allocate space for a blank Window class in memory
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