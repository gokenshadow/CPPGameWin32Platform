
/*
 TODO(casey): THIS IS NOT A FINAL PLATFORM LAYER
 - Saved game locations
 - Getting a handle to our own executable file
 - Asset loading path
 - Threading (launch a thread)
 - Raw Input (support for multiple keyboards)
 - Sleep/timeBeginPeriod
 - ClipCursor() (for multimonitor support)
 - Fullscreen support
 - WM_SETCURSOR (control cursor visibility)
 - QueryCancelAutoplay
 - WM_ACTIVATEAPP (for when we are not the active application)
 - Blit speed improvements (BitBlt)
 - Hardware acceleration (OpenGL or Direct3D or BOTH??)
 - GetKeyboardLayout (for French keyboards, international WASD support)
 
	Just a partial list of stuff!!
*/
#define internal static 
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

// Extra stuff for defs
#include <stdint.h>
#include <math.h>


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

//TODO(casey): Implement sine ourselves

#include "handmade.h"
#include "handmade.cpp"

#include <malloc.h>
// This will allow you to make windows create a window for you
#include <windows.h>

#include <stdio.h>
#include <Xinput.h>
#include <dsound.h>

struct win32_offscreen_buffer {
    // NOTE(casey): Pixels are always 32 bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

/// INPUT
// TODO(casey): This is a global for now.
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

// NOTE(Casey): XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(Casey): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuideDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


void * PlatformLoadFile(char *FileName) {
	//NOTE(casey): Implements the Win32 file loading
	return(0);
}


internal void Win32LoadXInput(void){
	// TODO(casey): Test this on Windows 8
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if(!XInputLibrary)
	{
		//TODO(casey): Diagnostic
		HMODULE XInputLibrary = LoadLibraryA("XInput9_1_0.dll");
	}
	if(!XInputLibrary)
	{
		//TODO(casey): Diagnostic
		HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if(XInputLibrary){
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		
		//TODO(casey): Diagnostic
	} else {
		//TODO(casey): Diagnostic
	}
}

/// Buffersize = 48000
/// Samples per second = 131070*48000 = 6,291,360,000

//// SOUND
internal void Win32InitDsound(HWND Window, int32 SamplesPerSecond, int32 BufferSize) {
	// NOTE(casey): Load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	
	if(DSoundLibrary){
		// NOTE(casey): Get a DirectSound object!
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		
		// TODO(casey) Double-check that this works on XP - DirectSound8 or 7??
		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample)/8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;
			if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))){
					
					DSBUFFERDESC BufferDescription = {};
					BufferDescription.dwSize = sizeof(BufferDescription);
					BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
					
					// NOTE(casey): "Create" a primary buffer
					// TODO(casey): DSBCAPS_GLOBALFOCUS?
					LPDIRECTSOUNDBUFFER PrimaryBuffer;
					if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer,0))) {
						
						if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))){
								// NOTE(casey): We have finally set the format!
								OutputDebugStringA("Primary buffer format was set.\n");
						} else {
							//TODO(casey): Diagnostic
						}
					}
			} else {
				//TODO(casey): Diagnostic
			}

			// TODO(casey) DSBCAPS_GETCURRENTPOSITOIN?
			// NOTE(casey): "Create" a secondary buffer
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			if(SUCCEEDED(Error)) {
					OutputDebugStringA("Secondary buffer created successfully.\n");
			}
				
			// NOTE(casey): Start it playing!
		} else {
			//TODO(casey): Diagnostic
		}		
	}
	
}


struct win32_window_dimension{
    int Width;
    int Height;
};

internal win32_window_dimension Win32GetWindowDimension (HWND Window){

    win32_window_dimension Result;
	
	// RECT is the data format that Windows uses to represent the dimensions of 
	// a window (HWND) that has been opened. 
	// v This command v allocates the amount of space needed for that RECT on the stack
    RECT ClientRect;
	
	// v This v is a windows.h command that will get the RECT of any given window 
	// (in our case the window would be "Window").
	// However, instead of RETURNING the value like any normal function would do, 
	// this function shoves the value into an already allocated space of memory
	// (in our case this space of memory would be the one that was created by our 
	// ^ RECT ClientRect; ^ command above)
    GetClientRect(Window, &ClientRect);
	
	// Now we're getting the width and height (which is all we want) from that
	// RECT
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
	
	// And now we're returning it
    return (Result);
}



internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height){
    // TODO: Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.
 

    // We want to clear the screen every time, so if there is already memory
    // in the bitmapmemory spot, clear it away
    if(Buffer->Memory){
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }


    // Fill out a bunch of fields
    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;

    // Say to Windows "We're gonna pass a bunch of memory and here's how you interpret it"
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height; //biHeight negative tells Windows top-down, not bottom-up
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    
    // Allocate the memory necessary to draw the entire screen. This will change everytime 
    // the screen size changes.
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    //RenderWeirdGradient(0,0);
    // TODO: Probably clear this to black

    Buffer->Pitch = Width*BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, 
    win32_offscreen_buffer *Buffer) {

    // TODO: Aspect ratio correction
	// StretchDIBits takes the color data (what colors the pixels are) of an image
	// And puts it into a destination 
    StretchDIBits(DeviceContext,
                    0, 0, WindowWidth, WindowHeight,
                    0, 0, Buffer->Width, Buffer->Height,
                    Buffer->Memory,
                    &Buffer->Info,
                    DIB_RGB_COLORS,
                    SRCCOPY
                    );
}


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
        } break;
        case WM_CLOSE:
        {
            GlobalRunning = false;
            PostQuitMessage(0);
        } break;
        case WM_ACTIVATEAPP:
        {
        } break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:{
			uint32 VKCode = WParam;
			bool WasDown = ((LParam & (1 << 30)) != 0);
			bool IsDown = ((LParam & (1<< 31)) == 0);
			
			if(WasDown != IsDown) {
				if(VKCode == 'W') {
					
				} else if (VKCode == 'A') {
					
				} else if (VKCode == 'S') {
					
				} else if (VKCode =='D') {
					
				} else if (VKCode == 'A') {
					
				} else if (VKCode == 'E') {
					
				} else if (VKCode == VK_UP) {
					
				} else if (VKCode == VK_DOWN) {
					
				} else if (VKCode == VK_RIGHT) {
					
				} else if (VKCode == VK_LEFT) {
					
				} else if (VKCode == VK_ESCAPE) {
					printf("ESCAPE: ");
					if(IsDown) {
						uint32 VKCode = WParam;						
						GlobalRunning = false;
						PostQuitMessage(0);					
					}
					if(WasDown) {
					}
				} else if (VKCode == VK_SPACE) {
					
				}
				bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);
				if ((VKCode == VK_F4) && AltKeyWasDown){
					GlobalRunning = false;
				}	
			}
		} break;
		// WM_PAINT is the event that happens when the window starts to draw. It will rehappen every time
		// the user messes with the window (resizes it, moves it around, unminimizes it, etc
        case WM_PAINT:{
            
			// Allocate enough memory for a paintstruct
			PAINTSTRUCT Paint;
			
			// Get the DeviceContext by pointing Windows's BeginPaint function to that PAINTSTRUCT
            HDC DeviceContext = BeginPaint(Window, &Paint);
			
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            RECT ClientRect;
			
            // Get clientrect gives you the coordinates of the window you've created
            GetClientRect(Window, &ClientRect);

            Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
            EndPaint(Window, &Paint);
            
            // PTBLT for painting blinking black and white on the screen when you resize the window
            /*
            static DWORD Operation = BLACKNESS;
            PatBlt(DeviceContext, X, Y, Width, Height, Operation);
            if(Operation == BLACKNESS){
                Operation = WHITENESS;
            } else {
                Operation = BLACKNESS;
            }
            EndPaint(Window, &Paint);
            */
        } break;
        default:
        {
            //DefWindowsProc just returns the default windows procedure that would happen
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

struct win32_sound_output {
	// NOTE(casey): Sound Test
	int SamplesPerSecond;
	int ToneHz;
	int16 ToneVolume;
	uint32 RunningSampleIndex;
	int WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
	real32 tSine;
	int LatencySampleCount;
};

internal void Win32ClearSoundBuffer(win32_sound_output *SoundOutput){
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(
			 0, SoundOutput->SecondaryBufferSize,
			 &Region1, &Region1Size,
			 &Region2, &Region2Size,
			 0 
	))) {
		// TODO(casey): assert that Region1Size/Region2Size is valid
		uint8 *DestSample = (uint8 *)Region1;
		for(DWORD ByteIndex=0; ByteIndex < Region1Size; ++ByteIndex) {
			*DestSample++ = 0;
		}
		DestSample = (uint8 *)Region2;
		for(DWORD ByteIndex=0; ByteIndex < Region2Size; ++ByteIndex) {
			*DestSample++ = 0;
		}
		
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void Win32FillSoundBuffer (win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
									game_sound_output_buffer *SourceBuffer) {
	//TODO(casey): More strenuous test!
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;	
	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(
			 ByteToLock, BytesToWrite,
			 &Region1, &Region1Size,
			 &Region2, &Region2Size,
			 0 
	))) {				
		
		// TODO(casey): assert that Region1Size/Region2Size is valid
		
		//TODO(casey): Collapse these two loops
		DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
		int16 *DestSample = (int16 *)Region1;
		int16 *SourceSample = SourceBuffer->Samples;
		for(DWORD SampleIndex=0; SampleIndex < Region1SampleCount; ++SampleIndex) {
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}
		
		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
		DestSample = (int16 *)Region2;
		for(DWORD SampleIndex=0; SampleIndex < Region2SampleCount; ++SampleIndex){
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}
		
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

// This is the main function where the program begins. This program uses the WinMain()
// function instead of Main() because Main() opens up a console, and we just want to open
// a Windows window.
int CALLBACK WinMain(
    // HINSTANCE - Instance handle of the application. Windows is set up to allocate us 
    // our own virtual memory space. The HINSTANCE is the way we tell Windows that we
    // want to access addresses within that virtual memory 
    HINSTANCE Instance, 
    HINSTANCE PrevInstance, // null
    LPSTR CmdLine, // if any parameters are put in command line when the program is run
    int ShowCode // determines how app window will be displayed
) {
	
	// This is data for FPS and the like
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	
	// This is for getting input from a gamepad
	Win32LoadXInput();
	
    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	
    // Allocate space for a blank Window class in memory
    WNDCLASSA WindowClass = {};


    // Add properties to the Window class
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.cbWndExtra;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    // regeister the Window class, if regeistration fails, logging maybe?
    if(RegisterClassA(&WindowClass)){
        
        // create a window handle
        HWND Window = CreateWindowExA(
            0,                              //dwExstyle
            WindowClass.lpszClassName,      //lpClassName
            "Handmade Hero",                //lpWindowName - name of window
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

        if(Window){
            // NOTE(CASEY): Since we sepecified CS_OWNDC, we can just get one device
            // context and use it forever because we are not sharing it with anyone
            HDC DeviceContext = GetDC(Window);
            
			// NOTE(casey): Graphics Test
			int XOffset = 0;
            int YOffset = 0;
			
			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.ToneHz = 256;
			SoundOutput.ToneVolume = 3000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
			SoundOutput.BytesPerSample = sizeof(int16)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			Win32InitDsound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			//Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
			
			bool32 SoundIsPlaying = false;
			
            GlobalRunning = true;
			
			//TODO(casey): Pool with bitmap VirtualAlloc
			int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			
			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			uint64 LastCycleCount = __rdtsc();
			while(GlobalRunning){
				// Allocate enough memory for a MSG structure, so we can place the stuff GetMessage() 
                // spits out into it
                 MSG Message;

                // The GetMessage() function will reach into the inards of the window handle we just
                // created and grab whatever message (eg window resize, window close, etc..) is queued 
                // up next. It will then send the raw data of that message to the memory we allocated 
                // for the MSG structure above 
                while(PeekMessage(&Message,0,0,0,PM_REMOVE)){
                    if(Message.message == WM_QUIT){
                        GlobalRunning = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
				// TODO:(casey): Should we pull this more frequently?
				for (DWORD ControllerIndex = 0; ControllerIndex<XUSER_MAX_COUNT; ++ControllerIndex){
					XINPUT_STATE ControllerState;
					if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS){
						// NOTE(Casey): This controller is plugged in
						// TODO(Casey): See if ControllerState.dwPacketNumber increments too rapidly
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
						
						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
						
						int StickX = Pad->sThumbLX;
						int StickY = Pad->sThumbLY;
						
						// TODO(casey): We will do deadzone handling later using
						// XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE 7849
						// XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE 8689
						
						XOffset += StickX / 4096;
						YOffset += StickY / 4096;
						
						SoundOutput.ToneHz = 512 + (int)256.0f*((real32)StickY/32000.0f);
						SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
						
						if(AButton) {
						}
						
					} else {
						// NOTE(Casey): The controller is not available
					}
					
				}
				
				//TODO(casey): Make sure this is gaurded entirely
				DWORD ByteToLock = 0;
				DWORD TargetCursor = 0;
				DWORD BytesToWrite = 0;
				DWORD PlayCursor = 0;
				DWORD WriteCursor = 0;
				bool32 SoundIsValid = false;
				//TODO(casey): Tighten up sound logic so that we know where we should be writing to
				//and can anticipate the time spent in the game update
				if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor))){
					ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample))
						% SoundOutput.SecondaryBufferSize;
					if(ByteToLock > TargetCursor){
						BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					} else {
						BytesToWrite = TargetCursor - ByteToLock;
					}
					SoundIsValid = true;
				}
				
				game_sound_output_buffer SoundBuffer = {};
				SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
				SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
				SoundBuffer.Samples = Samples;
				
				game_offscreen_buffer Buffer = {};
				Buffer.Memory = GlobalBackBuffer.Memory;
				Buffer.Width = GlobalBackBuffer.Width;
				Buffer.Height = GlobalBackBuffer.Height;
				Buffer.Pitch = GlobalBackBuffer.Pitch;
				GameUpdateAndRender(&Buffer, XOffset, YOffset, &SoundBuffer,SoundOutput.ToneHz);
                //RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);
				
				
				// NOTE(casey): DirectSound output test
				if(SoundIsValid){
					Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
					
					//  int16 int16   int16 int16   int16 int16  ...
					// [LEFT  RIGHT] [LEFT  RIGHT] [LEFT  RIGHT] ...
				}
				
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
                ReleaseDC(Window, DeviceContext);
				
				uint64 EndCycleCount = __rdtsc();
                
				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);
				
				//MainLoop();
				
				uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
				uint64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				real32 MSPerFrame = (((1000.0f*(real32)CounterElapsed) / (real32)PerfCountFrequency));
				real32 FPS = (real32)PerfCountFrequency / (real32)CounterElapsed;
				real32 MCPF = ((real32)CyclesElapsed / (1000.0f * 1000.0f));
				
#if 0
				char Buffer[256];
				sprintf(Buffer, " %.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
				OutputDebugStringA(Buffer);
#endif				
				LastCounter = EndCounter;
				LastCycleCount = EndCycleCount;
            }
        } else {
            // TODO(Casey): Logging    
        }
    } else {
        // TODO(Casey): Logging
    }   

    return(0);
}