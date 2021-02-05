
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

#include "win32_handmade.h"

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


internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename) {
	
	debug_read_file_result Result = {};
	// handle = usually indexes to a table, pointer into Kernel memory
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle, &FileSize)) {
			uint32 FileSize32 = SafeTruncateSize64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(Result.Contents) {
				DWORD BytesRead;
				if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) 
					&&FileSize32==BytesRead) {
					// Note(casey): File read successfully
					Result.ContentSize = FileSize32;
				} else {
					// TODO(casey): Logging
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			} else {
				// TODO(casey): Logging
			}
		} else {
			// TODO(casey): Logging
		}
		
		CloseHandle(FileHandle);
	}
	return(Result);
}

internal void DEBUGPlatformFreeFileMemory(void *Memory) {
	if(Memory) {
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}
internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory) {
	bool32 Result = false;
	// handle = usually indexes to a table, pointer into Kernel memory
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE) {
		DWORD BytesWritten;
		if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) {
			// Note(casey): File written successfully
			Result = (BytesWritten == MemorySize);
		} else {
			// TODO(casey): Logging
		}
		
		CloseHandle(FileHandle);
	}
	return(Result);
}


internal void Win32LoadXInput(void){
	// TODO(casey): Test this on Windows 8
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if(!XInputLibrary)
	{
		//TODO(casey): Diagnostic
		XInputLibrary = LoadLibraryA("XInput9_1_0.dll");
	}
	if(!XInputLibrary)
	{
		//TODO(casey): Diagnostic
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
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

// This function is where the allocation of necessary memory for the pixels that go on the screen happens
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
    Buffer->Info.bmiHeader.biCompression = BI_RGB; // No compression
    
	// Here we are calculating the amount of memory we need in bytes to have in order to fill each pixel,
	// This will change based on the width and height and bytesperpixel, etc
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
			Assert(!"Keyboard came in through a non-dispatch message!");
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

internal void Win32ProcessKeyboardMessage (game_button_state *NewState, bool32 IsDown) {
	Assert(NewState->EndedDown != IsDown);
	NewState->EndedDown = IsDown; 
	++NewState->HalfTransitionCount;
}

internal void Win32ProcessXInputDigitalButtton (DWORD XInputButtonState, game_button_state *OldState, 
												DWORD ButtonBit, game_button_state *NewState) {
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal real32 Win32ProcessXInputStickValue (short Value, short DeadZoneThreshold) {
	real32 Result  = 0;
	if(Value < -DeadZoneThreshold){
		Result = (real32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));							
	} else if (Value > DeadZoneThreshold) {
		Result = (real32)((Value + DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
	}
	return(Result);
}

internal void Win32ProcessHandlingMessages (game_controller_input *KeyboardController){
	// The GetMessage() function will reach into the inards of the window handle we just
	// created and grab whatever message (eg window resize, window close, etc..) is queued 
	// up next. It will then send the raw data of that message to the memory we allocated 
	// for the MSG structure above 
	MSG Message;
	while(PeekMessage(&Message,0,0,0,PM_REMOVE)){
		switch(Message.message) {
			case WM_QUIT: {
				GlobalRunning = false;
			} break;
			
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:{ 
				uint32 VKCode = (uint32)Message.wParam;
				bool WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool IsDown = ((Message.lParam & (1<< 31)) == 0);			
				if(WasDown != IsDown) {
					if(VKCode == 'W') {
						Win32ProcessKeyboardMessage (&KeyboardController->ActionUp, IsDown);
					} else if (VKCode == 'A') {
						Win32ProcessKeyboardMessage (&KeyboardController->ActionLeft, IsDown);
					} else if (VKCode == 'S') {
						Win32ProcessKeyboardMessage (&KeyboardController->ActionDown, IsDown);
					} else if (VKCode =='D') {
						Win32ProcessKeyboardMessage (&KeyboardController->ActionRight, IsDown);
					} else if (VKCode == 'Q') {
						Win32ProcessKeyboardMessage (&KeyboardController->LeftShoulder, IsDown);
					} else if (VKCode == 'E') {
						Win32ProcessKeyboardMessage (&KeyboardController->RightShoulder, IsDown);
					} else if (VKCode == VK_UP) {
						Win32ProcessKeyboardMessage (&KeyboardController->MoveUp, IsDown);
					} else if (VKCode == VK_DOWN) {
						Win32ProcessKeyboardMessage (&KeyboardController->MoveDown, IsDown);
					} else if (VKCode == VK_RIGHT) {
						Win32ProcessKeyboardMessage (&KeyboardController->MoveRight, IsDown);
					} else if (VKCode == VK_LEFT) {
						Win32ProcessKeyboardMessage (&KeyboardController->MoveLeft, IsDown);
					} else if (VKCode == VK_ESCAPE) {
						GlobalRunning=false;
						Win32ProcessKeyboardMessage (&KeyboardController->Start, IsDown);
					} else if (VKCode == VK_SPACE) {
						Win32ProcessKeyboardMessage (&KeyboardController->Back, IsDown);
					}
					bool AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
					if ((VKCode == VK_F4) && AltKeyWasDown){
						GlobalRunning = false;
					}	
				}
			
			} break;
			
			default: {
				TranslateMessage(&Message);
				DispatchMessage(&Message);								
			} break;
		}
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
	
	// This allocates the memory needed for the window based on the height
	// It will currently take the GlobalBackBuffer struct and 
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
			
			
			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
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
			
#if HANDMADE_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terrabytes(2);			
#else
			LPVOID BaseAddress = 0;
#endif
			
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(1);
			
			// TODO (casey): Handle various memory footprints (USING SYSTEM METRUCS)
			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (SIZE_T)TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
			
			if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage) {
					
				
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];
				
				LARGE_INTEGER LastCounter;
				QueryPerformanceCounter(&LastCounter);
				uint64 LastCycleCount = __rdtsc();
				while(GlobalRunning){	
				
					game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					game_controller_input ZeroController = {};
					*NewKeyboardController = ZeroController;
					NewKeyboardController->IsConnected = true;
					
					for(int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++){
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}
					
					Win32ProcessHandlingMessages(NewKeyboardController);
						
					// TODO:(casey) Need to not pull disconnected controllers to avoid
					// xinput framerate hit on older libraries...
					// TODO:(casey): Should we pull this more frequently?
					DWORD MaxControllerCount = XUSER_MAX_COUNT;
					if(MaxControllerCount > (ArrayCount(NewInput->Controllers) -1)){
						MaxControllerCount = (ArrayCount(NewInput->Controllers) -1);
					}
					for (DWORD ControllerIndex = 0; ControllerIndex<MaxControllerCount; ++ControllerIndex){
						DWORD OurControllerIndex = ControllerIndex + 1;
						game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
						game_controller_input *NewController = GetController(NewInput, OurControllerIndex);
						
						
						XINPUT_STATE ControllerState;
						if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS){
							NewController->IsConnected = true;
							// NOTE(Casey): This controller is plugged in
							// TODO(Casey): See if ControllerState.dwPacketNumber increments too rapidly
							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
							
							// TODO(Casey): This is a square deadzone, check xinput to verify that the
							// deadzone is "round" and show how to do round deadzone processing.
							NewController->IsAnalog = true;
							NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							if(NewController->StickAverageX != 0.0f || NewController->StickAverageY != 0.0f){
								NewController->IsAnalog=true;
							}
							if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
								NewController->StickAverageY = 1;
								NewController->IsAnalog = false;
							}
							if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
								NewController->StickAverageY = -1;
								NewController->IsAnalog = false;
							}
							if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
								NewController->StickAverageX = -1;
								NewController->IsAnalog = false;
							}
							if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
								NewController->StickAverageX = 1;
								NewController->IsAnalog = false;
							}
							
							real32 Threshold = 0.5f;
							Win32ProcessXInputDigitalButtton ((NewController->StickAverageX < -Threshold) ? 1 : 0, 
																&OldController->MoveLeft, 1, 
																&NewController->MoveLeft);
																
							Win32ProcessXInputDigitalButtton ((NewController->StickAverageX > Threshold) ? 1 : 0, 
																&OldController->MoveRight, 1, 
																&NewController->MoveRight);
																
							Win32ProcessXInputDigitalButtton ((NewController->StickAverageY < -Threshold) ? 1 : 0, 
																&OldController->MoveDown, 1, 
																&NewController->MoveDown);
																
							Win32ProcessXInputDigitalButtton ((NewController->StickAverageY > Threshold) ? 1 : 0, 
																&OldController->MoveUp, 1, 
																&NewController->MoveUp);
							Win32ProcessXInputDigitalButtton (Pad->wButtons, 
																&OldController->ActionDown, XINPUT_GAMEPAD_A, 
																&NewController->ActionDown);
							Win32ProcessXInputDigitalButtton (Pad->wButtons, 
																&OldController->ActionRight, XINPUT_GAMEPAD_B, 
																&NewController->ActionRight);
							Win32ProcessXInputDigitalButtton (Pad->wButtons, 
																&OldController->ActionLeft, XINPUT_GAMEPAD_X, 
																&NewController->ActionLeft);
							Win32ProcessXInputDigitalButtton (Pad->wButtons, 
																&OldController->ActionUp, XINPUT_GAMEPAD_Y, 
																&NewController->ActionUp);
							Win32ProcessXInputDigitalButtton (Pad->wButtons, 
																&OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, 
																&NewController->RightShoulder);
							Win32ProcessXInputDigitalButtton (Pad->wButtons, 
																&OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, 
																&NewController->LeftShoulder);
							Win32ProcessXInputDigitalButtton (Pad->wButtons, 
																&OldController->Start, XINPUT_GAMEPAD_START, 
																&NewController->Start);
							Win32ProcessXInputDigitalButtton (Pad->wButtons, 
																&OldController->Back, XINPUT_GAMEPAD_BACK, 
																&NewController->Back);

							
							//bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
							//bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
							
						} else {
							// NOTE(Casey): The controller is not available
							NewController->IsConnected = false;
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
					GameUpdateAndRender(&GameMemory, Input, &Buffer, &SoundBuffer);
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
					
					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
					
					//TODO(casey): Should I clear these here?
					
				}
			} else {
				// TODO(Casey): Logging can't get memory
			}
        } else {
            // TODO(Casey): Logging    
        }
    } else {
        // TODO(Casey): Logging
    }   

    return(0);
}