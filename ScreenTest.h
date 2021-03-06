#if !defined(WIN32_HANDMADE_H)

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 25;}

struct game_offscreen_buffer {
    // NOTE(casey): Pixels are always 32 bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
	int BytesPerPixel;
};

void b();

inline uint32 SafeTruncateSize64(uint64 Value) {
	//TODO(casey): Defines for maximum values
	Assert(Value <= 0xFFFFFF);
	uint32 Result = (uint32)Value;
	return Result;
}

struct bmp_pixel {
	uint8 blue;
	uint8 green;
	uint8 red;
};

struct bmp_image_data {
	uint32 Width;
	uint32 Height;
	uint32 BytesPerPixel;
	uint32 Size;
	uint8 *ImageData;
};

struct debug_read_file_result {
	uint32 ContentSize;
	void *Contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(const char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(const char *Filename, uint32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);


#define GET_BMP_IMAGE_DATA(name) bmp_image_data name(const char *Filename)
typedef GET_BMP_IMAGE_DATA(get_bmp_image_data);
#define CLEAR_BMP_IMAGE_DATA(name) void name(bmp_image_data BmpToClear)
typedef CLEAR_BMP_IMAGE_DATA(clear_bmp_image_data);

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
	int MouseDeltaX;
	int MouseDeltaY;
	
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
	
	get_bmp_image_data *GetBmpImageData;
	clear_bmp_image_data *ClearBmpImageData;
	
	debug_platform_free_file_memory  *DEBUGPlatformFreeFileMemory;
	debug_platform_read_entire_file  *DEBUGPlatformReadEntireFile; 
	debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
	
};

#define WIN32_HANDMADE_H
#endif