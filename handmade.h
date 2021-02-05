#if !defined(HANDMADE_H)

/*
	NOTE(casey):
	
	HANDMADE_INTERNAL:
		0 - Build for public release
		1 - Build for developer only
		
	HANDMADE_SLOW:
		0 - No slow code allowed!
		1 - Slow code welcome.
*/


#if HANDMADE_SLOW
// v This macro v purposefully stops the program if the Expression is false. 
// {*(int *)0 = 25;} is writing the value 25 to the null ptr 
// (address 0 aka 0x00000000), which is an illegal thing to do and will 
// crash the code, giving an error in the debugger
// TODO(casey): Complete Assertion macro - don't worry everyone!
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 25;}
#else
#define Assert(Expression)
#endif

// TODO(casey): Should these always be 64 bit?
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terrabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
// TODO(Casey): swap, min, max ... macros???
inline uint32 SafeTruncateSize64(uint64 Value) {
	//TODO(casey): Defines for maximum values
	Assert(Value <= 0xFFFFFF);
	uint32 Result = (uint32)Value;
	return Result;
}

/*
	TODO(casey): Services that the game provides to the platform layer.
*/
#if HANDMADE_INTERNAL
/* IMPORTANT(casey):

	These are NOT for doing anything in the shipping game - they are blocking
	and the write doesn't protect against lost data!
*/
struct debug_read_file_result {
	uint32 ContentSize;
	void *Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory);
#endif


/*
	NOTE(casey): Services that the platform layer provides to the game
	(this may expand in the future - sound on separate thread, etc.)
*/

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

// TODO(casey): In the future, rendering specificallly will become a three-teired abstraction!
struct game_offscreen_buffer {
    // NOTE(casey): Pixels are always 32 bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_sound_output_buffer {
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
};

struct game_button_state {
	int HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input {
	bool32 IsAnalog;
	real32 StickAverageX;
	real32 StickAverageY;
	
	union {
		game_button_state Buttons[10];
		struct {
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;
			
			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;
			
			game_button_state LeftShoulder;
			game_button_state RightShoulder;		
		};
	};
};

struct game_input {
	// TODO(casey): Insert clock values here.
	game_controller_input Controllers[5];
};



struct game_memory {
	bool32 IsInitialized;
	uint64 PermanentStorageSize;
	void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup
	
	uint64 TransientStorageSize;
	void *TransientStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup
};

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, 
								  game_sound_output_buffer *SoundBuffer);
//
//
//
struct game_state {
	int ToneHz;
	int GreenOffset;
	int BlueOffset;
};

#define HANDMADE_H
#endif
