#if !defined(WIN32_HANDMADE_H)

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 25;}

struct game_offscreen_buffer {
    // NOTE(casey): Pixels are always 32 bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_state {
	bool32 IsInitialized;
	int ToneHz;
	int XOffset;
	int YOffset;
	real32 tSine;
	int XSpeed;
	int YSpeed;
	int ToneLevel;
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
	bool32 IsConnected;
	bool32 IsAnalog;
	real32 StickAverageX;
	real32 StickAverageY;
	
	union {
		game_button_state Buttons[12];
		struct {
			bool MoveUp;
			bool MoveDown;
			bool MoveLeft;
			bool MoveRight;
			
			bool ActionUp;
			bool ActionDown;
			bool ActionLeft;
			bool ActionRight;
			
			bool LeftShoulder;
			bool RightShoulder;		
			
			bool Back;
			bool Start;
			
			// NOTE(casey): All buttons must be added above this line
			
			game_button_state Terminator;
		};
	};
};

struct game_memory {
	bool IsInitialized;
	uint64 PermanentStorageSize;
	void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup
	
	uint64 TransientStorageSize;
	void *TransientStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup
	
};

#define WIN32_HANDMADE_H
#endif