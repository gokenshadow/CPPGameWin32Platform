#if !defined(HANDMADE_H)

/*
	TODO(casey): Services that the game provides to the platform layer.
*/

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

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, 
								  int GreenOffset, game_sound_output_buffer *SoundBuffer,
								  int ToneHz);

#define HANDMADE_H
#endif
