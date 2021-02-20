#if !defined(WIN32_HANDMADE_H)

struct win32_offscreen_buffer {
    // NOTE(casey): Pixels are always 32 bits wide, Memory Order BB GG RR XX
	
	// A DIB is like a BMP, datawise. The BITMAPINFO is a struct that contains stuff about that DIB.
	// Inside BITMAPINFO is the bmiHeader, which will be very much like the
	// header of our imaginary BMP, i.e. all the information about how many colors it
	// will display (8 bit, 16 bit, 32 bit, etc), its width, it's height, etc..
    BITMAPINFO Info; // it's not really the whole BITMAPINFO we care about, just the header
    void *Memory; 	// this be the memory where the data of the DIB is stored; 
	// this ^ variable is not the memory itself, but a pointer to the location of the memory
    int Width;
    int Height;
    int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension {
    int Width;
    int Height;
};

struct win32_sound_output {
	// NOTE(casey): Sound Test
	int SamplesPerSecond;
	uint32 RunningSampleIndex;
	int WavePeriod;
	int BytesPerSample;
	DWORD SecondaryBufferSize;
	DWORD SafetyBytes;
	real32 tSine;
	int LatencySampleCount;
	//TODO(Casey): Math gets simpler if we add a "BytesPerSecond" field?
	//TODO(Casey): Should RunningSampleIndex be in bytes as well?
};

struct win32_debug_time_marker {
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;
	DWORD OutputLocation;
	DWORD OutputByteCount;
	DWORD ExpectedFlipPlayCursor;
	
	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;
};

struct win32_game_code {
	HMODULE GameCodeDLL;
	FILETIME DLLLastWriteTime;
	
	// IMPORTANT(casey): Either of the callbacks can be 0!
	// You must check before calling.
	game_get_sound_samples *GetSoundSamples;
	game_update_and_render *UpdateAndRender;
	
	bool32 IsValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_state {
	uint64 TotalSize;
	void *GameMemoryBlock;
	HANDLE RecordingHandle;
	int InputRecordingIndex;
	
	HANDLE PlaybackHandle;
	int InputPlayingIndex;
	char EXEFilename[WIN32_STATE_FILE_NAME_COUNT];
	char *OnePastLastEXEFileNameSlash;
};



#define WIN32_HANDMADE_H
#endif
