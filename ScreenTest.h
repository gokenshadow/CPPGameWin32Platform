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
};

#define WIN32_HANDMADE_H
#endif