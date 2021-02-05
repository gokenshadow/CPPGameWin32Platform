// We want to be explicit about the number of bytes we're allocating in our variables, so we're
// importing more explicit definitions
#include <stdint.h>

// Defining these this way because it's easier to understand drawing to the screen
typedef uint8_t uint8;
typedef uint32_t uint32;

// This will allow you to make windows create a window for you
#include <windows.h>

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
static win32_offscreen_buffer GlobalBackBuffer;
static void *GlobalDibMemory;

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
            CW_USEDEFAULT,                  //nWidth
            CW_USEDEFAULT,                  //nHeight
            0,                              //hWndParent - 0 if only one window
            0,                              //hMenu 0 - if not using any windows menus
            Instance,                       //hInstance - the handle to the app instance
            0                               //lpParam
        );

        if(Window){
            // The Device Context is the place in memory where the drawing will go. 
			// v This function v will temporarily grab the Device Context of the Window we've just created, so we can draw into it
            HDC DeviceContext = GetDC(Window);
           
			// Set up variables to make the weird gradient move
			int XOffset = 0;
            int YOffset = 0;
			
            GlobalRunning = true;
			
			while(GlobalRunning){
				// Allocate enough memory for a MSG structure, so we can place the stuff GetMessage() 
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
									YOffset+=40;
								} else if (VKCode == VK_DOWN) {
									YOffset-=40;
								} else if (VKCode == VK_RIGHT) {
								} else if (VKCode == VK_LEFT) {
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
								TranslateMessage(&Message);
								DispatchMessage(&Message);								
						} break;
					}
                }
				
				
				// RENDER WEIRD GRADIENT
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
				
				// StretchDIBits takes the color data (what colors the pixels are) of an image
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
				XOffset++;
				//YOffset++;
				
				// It is said that if you Get a DC, you must release it when you're done with it. I have no idea why.
				ReleaseDC(Window, DeviceContext);
				
            }
        } else {
            // TODO(Casey): Logging    
        }
    } else {
        // TODO(Casey): Logging
    }   

    return(0);
}