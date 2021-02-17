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