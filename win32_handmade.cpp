
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
// Extra stuff for defs

#include "handmade.h"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <Xinput.h>
#include <dsound.h>

#define Win32GameWidth 1280
#define Win32GameHeight 720

#include "win32_handmade.h"

/// INPUT
// TODO(casey): This is a global for now.
global_variable bool GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;

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


internal void CatStrings (size_t SourceACount, char *SourceA,
						 size_t SourceBCount, char *SourceB,
						 size_t DestCount, char *Dest){
	// TODO(casey): Dest bounds checking!
	for (size_t i = 0; i < SourceACount; i++) {
		*Dest++ = *SourceA++;
	}
	
	for (size_t i = 0; i < SourceACount; i++) {
		*Dest++ = *SourceB++;
	}
	// All strings in C end with the null something (0 aka null)
	*Dest++ = 0;
	
}

internal void Win32GetEXEFilename (win32_state *State) {
	// NOTE(casey): Never use MAX_PATH in code that is user-facing, because it can be
	// dangerous and lead to bad results.;
	DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEFilename, sizeof(State->EXEFilename));
	State->OnePastLastEXEFileNameSlash = State->EXEFilename;
	for(char *Scan = State->EXEFilename; *Scan; ++Scan) {
		if(*Scan == '\\'){
			State->OnePastLastEXEFileNameSlash = Scan + 1;
		}
	}
}

internal int StringLength(char *String) {
	int Count = 0;
	while(*String++) {
		++Count;
	}
	return(Count);
}

internal void Win32BuildEXEPathFilename (win32_state *State, char *Filename, int DestCount, char *Dest) {	
	CatStrings (State->OnePastLastEXEFileNameSlash - State->EXEFilename, State->EXEFilename,
				StringLength(Filename), Filename,
				DestCount, Dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory) {
	if(Memory) {
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile) {	
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
					DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
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
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile) {
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

inline FILETIME Win32GetLastWriteTime(char *Filename) {
	FILETIME LastWriteTime = {};
	
	WIN32_FILE_ATTRIBUTE_DATA Data;
	if(GetFileAttributesExA(Filename, GetFileExInfoStandard, &Data)) {
		LastWriteTime = Data.ftLastWriteTime;
	}	
	return(LastWriteTime);
}

internal win32_game_code Win32LoadGameCode(char *SourceDLLName,  char *TempDLLName){
	win32_game_code Result = {};
	
	//TODO(casey): Need to get the proper path here!
	//TODO(casey): Automatic determination of when updates are necessary.
	
	Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);
	
	CopyFile(SourceDLLName, TempDLLName, FALSE);
	Result.GameCodeDLL = LoadLibraryA(TempDLLName);
	if(Result.GameCodeDLL) {
		Result.UpdateAndRender = (game_update_and_render *)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
		Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
		Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
	}
	if(!Result.IsValid) {		
		Result.UpdateAndRender = 0;
		Result.GetSoundSamples = 0;
	}

	return(Result);
}
internal void Win32UnloadGameCode (win32_game_code *GameCode){
	if(GameCode->GameCodeDLL){
		FreeLibrary(GameCode->GameCodeDLL);
		GameCode->GameCodeDLL = 0;
	}
	GameCode->IsValid = false;
	GameCode->UpdateAndRender = 0;
	GameCode->GetSoundSamples = 0;
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
			BufferDescription.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY;
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
	Buffer->BytesPerPixel = BytesPerPixel;

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
	// StretchDIBits takes the color data (what colors the pixels are) of an image
	// And puts it into a destination
	// Note(casey): For prototyping purposes, we're going to always blit 1-to-1
	// pixels to make sure we don't introduce artifacts with stretching while
	// we are learning to code the renderer!
    StretchDIBits(DeviceContext,
                    0, 0, Buffer->Width, Buffer->Height,
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
			#if 0
			if(WParam == true) {
				SetLayeredWindowAttributes(Window, RGB(0,0,0), 255, LWA_ALPHA);
			} else {				
				SetLayeredWindowAttributes(Window, RGB(0,0,0), 64, LWA_ALPHA);
			}
			#endif
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
	if(NewState->EndedDown != IsDown) {
		NewState->EndedDown = IsDown; 
		++NewState->HalfTransitionCount;		
	}
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

internal void Win32GetInputFileLocation(win32_state *State, bool32 InputStream, int SlotIndex, int DestCount, char *Dest) {
	char Temp[64];
	wsprintf(Temp, "loop_edit_%d_%s.hmi", SlotIndex, InputStream ? "input" : "state" );
	Win32BuildEXEPathFilename (State, Temp, DestCount, Dest);
}

internal win32_replay_buffer * Win32GetReplayBuffer(win32_state *State, int unsigned Index){
	Assert(Index < ArrayCount(State->ReplayBuffers));
	win32_replay_buffer *Result = &State->ReplayBuffers[Index];
	return(Result);
}

internal void Win32BeginRecordingInput (win32_state *State, int InputRecordingIndex){
	win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);
	if(ReplayBuffer->MemoryBlock) {
		State->InputRecordingIndex = InputRecordingIndex;
		
		
		char Filename[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(Filename), Filename);
		State->RecordingHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
		
		//DWORD BytesWritten;
		//WriteFile(State->RecordingHandle, State->GameMemoryBlock, BytesToWrite, &BytesWritten, 0);
		#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
		#endif
		CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);	
	}
}

internal void Win32EndRecordingInput (win32_state *State){
	CloseHandle(State->RecordingHandle);
	State->InputRecordingIndex = 0;
}

internal void Win32BeginInputPlayback (win32_state *State, int InputPlayingIndex){
	win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputPlayingIndex);
	if(ReplayBuffer->MemoryBlock) {
		State->InputPlayingIndex = InputPlayingIndex;
		
		char Filename[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, InputPlayingIndex, sizeof(Filename), Filename);
		State->PlaybackHandle = CreateFileA(Filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
#if 0		
		//DWORD BytesRead;
		//ReadFile(State->PlaybackHandle, State->GameMemoryBlock, BytesToRead, &BytesRead, 0);
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->PlaybackHandle, FilePosition, 0, FILE_BEGIN);
#endif		
		CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
	}
}

internal void Win32EndInputPlayback (win32_state *State){
	CloseHandle(State->PlaybackHandle);
	State->InputPlayingIndex = 0;
}

internal void Win32RecordInput(win32_state *State, game_input *NewInput) {
	DWORD BytesWritten;
	WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void Win32PlaybackInput(win32_state *State, game_input *NewInput) {
	DWORD BytesRead;
	if(ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0)) {
		if(BytesRead == 0) {
			// NOTE(casey): We've hit the end of the stream. Go back to the beginning.
			int PlayingIndex = State->InputPlayingIndex;
			Win32EndInputPlayback(State);
			Win32BeginInputPlayback(State, PlayingIndex);
			ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
		}		
	}
	else {
		
	}
}

internal void Win32ProcessPendingMessages (win32_state *State, game_controller_input *KeyboardController){
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
				// NOTE(casey): Since we are comparing WasDown to IsDown, we MUST use
				// == and != to convert these bit test to actual 0 or 1 values.
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
#if HANDMADE_INTERNAL					
					else if (VKCode == 'P') {
						if(IsDown) {
							GlobalPause = !GlobalPause;							
						}
						//Win32ProcessKeyboardMessage (&KeyboardController->RightShoulder, IsDown);
					} else if (VKCode == 'L') {
						if(IsDown) {							
							if(State->InputPlayingIndex == 0) {
								if(State->InputRecordingIndex == 0) {
									Win32BeginRecordingInput(State, 1);						
								} else {
									Win32EndRecordingInput(State);
									Win32BeginInputPlayback(State, 1);
								}
							} else {
								Win32EndInputPlayback(State);
							}
						}							
					}
#endif
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

inline LARGE_INTEGER Win32GetWallClock (void) {
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return (Result);
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End){
	real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency);
	return (Result);
}

internal void Win32DebugDrawVertical(win32_offscreen_buffer *BackBuffer, int X, int Top, int Bottom, uint32 Color) {
	if(Top <= 0) {
		Top = 0;
	}
	
	if(Bottom >= BackBuffer->Height) {
		Bottom = BackBuffer->Height;
	}
	
	if((X >= 0) && (X < BackBuffer->Width)) {
		uint8 *Pixel = (uint8 *)BackBuffer->Memory + X*BackBuffer->BytesPerPixel + Top*BackBuffer->Pitch;
		for (int Y = Top; Y < Bottom; ++Y) {
			*(uint32 *)Pixel = Color;
			Pixel += BackBuffer->Pitch;
		}
	}
}

inline void Win32DrawSoundBufferMarker (win32_offscreen_buffer *BackBuffer,
									win32_sound_output *SoundOutput, real32 C, 
									int PadX, int Top, int Bottom, DWORD Value, uint32 Color) {
	real32 XReal32 = (C * (real32)Value);
	int X = PadX + (int)XReal32;
	Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}

#if 0
internal void Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer, int MarkerCount, 
									win32_debug_time_marker *Markers,
									int CurrentMarkerIndex,
									win32_sound_output *SoundOutput, real32 TargetSecondsPerFrame) {
	
	int PadX = 16;
	int PadY = 16;
	
	int LineHeight = 64;
	
	real32 C = (real32)(BackBuffer->Width - 2*PadX) / (real32)SoundOutput->SecondaryBufferSize;
	for(int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex){
		win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
		Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputWriteCursor< SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);
		
		DWORD PlayColor = 0xFFFFFFFF;
		DWORD WriteColor = 0xFFFF0000;
		DWORD ExpectedFlipColor = 0x00000000;
		DWORD PlayWindowColor = 0xFFFFFF00;
			int Top = PadY;
			int Bottom = PadY + LineHeight;
		if(MarkerIndex == CurrentMarkerIndex) {
			Top += LineHeight+PadY;
			Bottom += LineHeight+PadY;

			int FirstTop = Top;
			
			Win32DrawSoundBufferMarker (BackBuffer,SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
			Win32DrawSoundBufferMarker (BackBuffer,SoundOutput, C, PadX ,Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);
			
			Top += LineHeight+PadY;
			Bottom += LineHeight+PadY;
			
			Win32DrawSoundBufferMarker (BackBuffer,SoundOutput, C, PadX ,Top, Bottom, ThisMarker->OutputLocation, PlayColor);
			Win32DrawSoundBufferMarker (BackBuffer,SoundOutput, C, PadX ,Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);
			
			Top += LineHeight+PadY;
			Bottom += LineHeight+PadY;
			
			Win32DrawSoundBufferMarker (BackBuffer,SoundOutput, C, PadX ,FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);

		}
		
		Win32DrawSoundBufferMarker (BackBuffer,SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
		Win32DrawSoundBufferMarker (BackBuffer,SoundOutput, C, PadX ,Top, Bottom, ThisMarker->FlipWriteCursor + 480*SoundOutput->BytesPerSample, PlayWindowColor);		
		Win32DrawSoundBufferMarker (BackBuffer,SoundOutput, C, PadX ,Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
	}
}
#endif
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

	win32_state State = {};
	Win32GetEXEFilename(&State);

	// This is data for FPS and the like
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	
	char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFilename (&State, "handmade.dll", sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);
	
	char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFilename (&State, "handmade_temp.dll", sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);
				
	// Note(Casey): Set the Windows scheduler granularity to 1ms, so that our Sleep()
	// can be more granular.
	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
	
	
	// This is for getting input from a gamepad
	Win32LoadXInput();
	
	// This allocates the memory needed for the window based on the width and height
    Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);
	
    // Allocate space for a blank Window class in memory
    WNDCLASSA WindowClass = {};
	
    // Add properties to the Window class
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.cbWndExtra;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	
    if(RegisterClassA(&WindowClass)){
        
        // create a window handle
        HWND Window = CreateWindowExA(
            0,//WS_EX_TOPMOST|WS_EX_LAYERED,                              //dwExstyle
            WindowClass.lpszClassName,      //lpClassName
            "Handmade Hero",                //lpWindowName - name of window
            WS_OVERLAPPEDWINDOW|WS_VISIBLE, //dwStyle
            CW_USEDEFAULT,                  //X
            CW_USEDEFAULT,                  //Y
            Win32GameWidth,			        //nWidth
            Win32GameHeight,           		//nHeight
            0,                              //hWndParent - 0 if only one window
            0,                              //hMenu 0 - if not using any windows menus
            Instance,                       //hInstance - the handle to the app instance
            0                               //lpParam
        );

        if(Window){
			// TODO(Casey): How to we reliably query on this on windows
			int MonitorRefreshHz = 60;
			HDC RefreshDC = GetDC(Window);
			int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
			ReleaseDC(Window, RefreshDC);
			
			if(Win32RefreshRate>1){
				MonitorRefreshHz = Win32RefreshRate;
			}
			real32 GameUpdateHz (MonitorRefreshHz / 2.0f);
			real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;
			
			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			// TODO(casey): Actually compute this variance and see what the lowest
			// reasonable value is.
			SoundOutput.SafetyBytes = (int)(((real32)SoundOutput.SamplesPerSecond*(real32)SoundOutput.BytesPerSample / GameUpdateHz) / 3.0f);
			Win32InitDsound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
			
			bool32 SoundIsPlaying = false;
			
            GlobalRunning = true;
#if 0
			// NOTE(casey): This tests the PlayCursor/WriteCursor update frequency
			// On the Handmade Hero machine, it was 480 samples.
			while(GlobalRunning) {
				DWORD PlayCursor;
				DWORD WriteCursor;
				GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
				
				char TextBuffer[256];
				sprintf_s(TextBuffer, sizeof(TextBuffer), "PC:%u WC:%u\n", PlayCursor, WriteCursor);
				OutputDebugStringA(TextBuffer);
			}
#endif		
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
			
			GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
			GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile; 
			GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
			
			// TODO (casey): Handle various memory footprints (USING SYSTEM METRUCS)
			// TODO (casey): Use MEM_LARGE_PAGES and call adjust token privileges when not on Windows XP?
			State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			State.GameMemoryBlock = VirtualAlloc(BaseAddress, (SIZE_T)State.TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			GameMemory.PermanentStorage = State.GameMemoryBlock;
			GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
			
			for(int ReplayIndex = 0; ReplayIndex < ArrayCount(State.ReplayBuffers); ReplayIndex++){
				win32_replay_buffer *ReplayBuffer = &State.ReplayBuffers[ReplayIndex];
				
				// TODO(Casey): Recording system still seems to take too long on record start - find out
				// what Windows is doing and see if we can speed up / defer some of that processing
				
				Win32GetInputFileLocation(&State, false, ReplayIndex, sizeof(ReplayBuffer->Filename), ReplayBuffer->Filename);
				
				ReplayBuffer->FileHandle = CreateFileA(ReplayBuffer->Filename, GENERIC_WRITE|GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);
				
				LARGE_INTEGER MaxSize;
				MaxSize.QuadPart = State.TotalSize;
				ReplayBuffer->MemoryMap = CreateFileMapping(ReplayBuffer->FileHandle, 0, PAGE_READWRITE, 
															MaxSize.HighPart, MaxSize.LowPart, 0);
															
				ReplayBuffer->MemoryBlock = MapViewOfFile(ReplayBuffer->MemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, State.TotalSize);
				if(ReplayBuffer->MemoryBlock) {
					
				} else {
					//TODO(Casey): Diagnostic
					
				}
			}
			
			
			if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage) {
					
				
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];
				NewInput->SecondsToAdvanceOverUpdate = TargetSecondsPerFrame;
				
				LARGE_INTEGER LastCounter = Win32GetWallClock();
				LARGE_INTEGER FlipWallClock = Win32GetWallClock();
				
				int DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[30] = {0};
				
				DWORD AudioLatencyBytes = 0;
				real32 AudioLatencySeconds = 0;
				bool32 SoundIsValid = false;
				
				win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
				uint32 LoadCounter = 0;
				
				uint64 LastCycleCount = __rdtsc();
				while(GlobalRunning){
					FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
					if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime)!=0) {
						Win32UnloadGameCode (&Game);
						Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
						LoadCounter = 0;
					}
					
					game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					game_controller_input ZeroController = {};
					*NewKeyboardController = ZeroController;
					NewKeyboardController->IsConnected = true;
					
					for(int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++){
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}
					
					Win32ProcessPendingMessages(&State, NewKeyboardController);
					
					if(!GlobalPause) {
						POINT MouseP;
						GetCursorPos(&MouseP);
						ScreenToClient(Window, &MouseP);
						NewInput->MouseX = MouseP.x;
						NewInput->MouseY = MouseP.y;
						NewInput->MouseZ = 0; // TODO(casey) Support mousewheel? 
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));					
						
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
								NewController->IsAnalog = OldController->IsAnalog;
								
								// NOTE(Casey): This controller is plugged in
								// TODO(Casey): See if ControllerState.dwPacketNumber increments too rapidly
								XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
								
								// TODO(Casey): This is a square deadzone, check xinput to verify that the
								// deadzone is "round" and show how to do round deadzone processing.
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
					
					thread_context Thread = {};
						
						game_offscreen_buffer Buffer = {};
						Buffer.Memory = GlobalBackBuffer.Memory;
						Buffer.Width = GlobalBackBuffer.Width;
						Buffer.Height = GlobalBackBuffer.Height;
						Buffer.Pitch = GlobalBackBuffer.Pitch;
						Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
						
						if(State.InputRecordingIndex) {
							Win32RecordInput(&State, NewInput);
						}
						
						if(State.InputPlayingIndex) {
							Win32PlaybackInput(&State, NewInput);
						}
						if(Game.UpdateAndRender) {							
							Game.UpdateAndRender(&Thread, &GameMemory, NewInput, &Buffer);
						}
						
						LARGE_INTEGER AudioWallClock = Win32GetWallClock();
						real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);
						
						DWORD PlayCursor;
						DWORD WriteCursor;
						if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor) == DS_OK){
							
							/*NOTE(casey): 
							Here is how sound output computation works.
							
							We define a safety value that is the number of samples (s) we think our game update loop
							may vary by (let's say up to 2ms)
							
							When we wake up to write audio, we will look and see what the play cursor position is:						
						  VidRender     frame 2                        frame 3                    frame 4
							   
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
							*/
			
							if(!SoundIsValid) {
								SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
								SoundIsValid = true;							
							}
							DWORD ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
							
							
							DWORD ExpectedSoundBytesPerFrame = (int)((real32)((SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz));
							
							real32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
							
							DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame)*(real32)ExpectedSoundBytesPerFrame);
							DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;
							
							DWORD SafeWriteCursor = WriteCursor;
							if(SafeWriteCursor < PlayCursor) {
								SafeWriteCursor += SoundOutput.SecondaryBufferSize;
							} 
							Assert(SafeWriteCursor >= PlayCursor);
							SafeWriteCursor += SoundOutput.SafetyBytes;
							
							bool32 AudioCardIsLowLatency = SafeWriteCursor < ExpectedFrameBoundaryByte;
							
							DWORD TargetCursor = 0;
							if(AudioCardIsLowLatency) {
								TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
							} else {
								TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + (SoundOutput.SafetyBytes));
							}
							TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;
							
							DWORD BytesToWrite = 0;
							if(ByteToLock > TargetCursor){
								BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
								BytesToWrite += TargetCursor;
							} else {
								BytesToWrite = TargetCursor - ByteToLock;
							}
							game_sound_output_buffer SoundBuffer = {};
							SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
							SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
							SoundBuffer.Samples = Samples;
							if(Game.GetSoundSamples) {
								Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);								
							}
	#if HANDMADE_INTERNAL
							win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
							Marker->OutputPlayCursor = PlayCursor;
							Marker->OutputWriteCursor = WriteCursor;
							Marker->OutputLocation = ByteToLock;
							Marker->OutputByteCount = BytesToWrite;
							Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte; 
							
							DWORD UnwrappedWriteCursor = WriteCursor;
							if(UnwrappedWriteCursor < PlayCursor){
								UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
								
							}
							DWORD AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
							real32 AudioLatencySeconds = (((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) / 
															(real32)SoundOutput.SamplesPerSecond);
							#if 0
							char TextBuffer[256];
							sprintf_s(TextBuffer, sizeof(TextBuffer), "BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA: %u (%fs)\n", 
											ByteToLock, TargetCursor, BytesToWrite, 
											PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
							OutputDebugStringA(TextBuffer);
							SoundIsValid = true;
							#endif
	#endif
							Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
						} else {
							SoundIsValid = false;
						}
						
						LARGE_INTEGER WorkCounter = Win32GetWallClock();
						real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
						
						// TODO(casey): NOT TESTED YET! PROBABLY BUGGY!!!
						real32 SecondsElapsedForFrame = WorkSecondsElapsed;
						
						real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,Win32GetWallClock());
						if(SecondsElapsedForFrame < TargetSecondsPerFrame) {
							if(SleepIsGranular) {
								DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
								if(SleepMS > 0){
									Sleep(SleepMS);								
								}
							}
							
							if(TestSecondsElapsedForFrame < TargetSecondsPerFrame) {
								//TODO(casey): LOG MISSED SLEEP HERE
								//Todo(Casey): Logging
							}
							while(SecondsElapsedForFrame < TargetSecondsPerFrame) {
								SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
							}						
						} else {
							//TODO(casey): MISSED FRAME RATE!
							//TODO(casey): Logging
						}
						
						LARGE_INTEGER EndCounter = Win32GetWallClock();
						real32 MSPerFrame = 1000.0f*Win32GetSecondsElapsed(EndCounter, LastCounter);
						LastCounter = EndCounter;
						
						
						win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	#if	HANDMADE_INTERNAL
						//NOTE(casey): Note, this is currently wrong on the zero'th index
						//Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DebugTimeMarkers), 
						//					DebugTimeMarkers, DebugTimeMarkerIndex-1, &SoundOutput, TargetSecondsPerFrame);

	#endif
						HDC DeviceContext = GetDC(Window);
						Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
						ReleaseDC(Window, DeviceContext);
	#if	HANDMADE_INTERNAL

						// NOTE(casey) This is debug code
						{
							DWORD PlayCursor;
							DWORD WriteCursor;
							if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor) == DS_OK){
								Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
								win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
								Marker->FlipPlayCursor = PlayCursor;
								Marker->FlipWriteCursor = WriteCursor;
							}
							
						}

	#endif
						ReleaseDC(Window, DeviceContext);
						
						game_input *Temp = NewInput;
						NewInput = OldInput;
						OldInput = Temp;
						//TODO(casey): Should I clear these here?
						#if 0
						uint64 EndCycleCount = __rdtsc();
						uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
						LastCycleCount = EndCycleCount;
						
						real32 FPS = 0.0f;
						real32 MCPF = ((real32)CyclesElapsed / (1000.0f * 1000.0f));
		
						char FPSBuffer[256];
						sprintf_s(FPSBuffer, " %.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
						OutputDebugStringA(FPSBuffer);
						#endif
	#if	HANDMADE_INTERNAL					
						++DebugTimeMarkerIndex;
						if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers)) {
							DebugTimeMarkerIndex = 0;
						}
	#endif				
					}	
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