#include "handmade.h"

internal void RenderWeirdGradient (game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
    uint8 *Row = (uint8*)Buffer->Memory;
    for(int Y = 0; Y<Buffer->Height; ++Y){
        uint32 *Pixel = (uint32 *)Row;
        for(int X = 0; X < Buffer->Width;
        ++X) {
            /*
                Pixel in Memory: BB GG RR xx
                LITTLE ENDIAN ARCHITECTURE

                0x xxBBGGRR 
            */
           uint8 Blue = (uint8)(X + XOffset);
           uint8 Green = (uint8)(Y + YOffset);
		   uint8 Red = 	(uint8)(X + XOffset);

           /*
            Memory:     BB GG RR xx
            Register:   xx RR GG BB
           */

            *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
        }
        Row += Buffer->Pitch;
    }
}

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz){
	
	local_persist real32 tSine;
	int16 ToneVolume = 3000;
	//int ToneHz = 256;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	
	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex=0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
		//TODO(casey): Draw this out for people
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		
		tSine += 2.0f*Pi32*1.0f/ (real32)WavePeriod;
	}
}



internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, 
								  game_sound_output_buffer *SoundBuffer) {
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized) {
		
		char *Filename = __FILE__;
		
		debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
		if(File.Contents) {
			DEBUGPlatformWriteEntireFile("test.out", File.ContentSize, File.Contents);
			DEBUGPlatformFreeFileMemory(File.Contents);			
		}
		
				
		GameState->ToneHz = 256;
		
		// TODO(casey): This may be more appropriate to do in the platform layer 
		Memory->IsInitialized = true;
	}
	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex) {
		game_controller_input *Controller = &Input->Controllers[ControllerIndex];
		if(Controller->IsAnalog){
			// NOTE:(casey) Use analoy movement tuning
			GameState->BlueOffset += (int)(4.0f*(Controller->EndX));
			GameState->ToneHz = 256 + (int)(128.0f*(Controller->EndY));
		} else {
			// Use digital movement tuning
		}
		
		//Input.AButtonEndedDown;
		//Input.AButtonHalfTransitionCount;
		if(Controller->Down.EndedDown) {
			GameState->GreenOffset += 1;
		}		
	}
	
	
	
	
	// TODO(casey): Allow sample offsets here for more robust platform options
	GameOutputSound(SoundBuffer, GameState->ToneHz);
	RenderWeirdGradient (Buffer, GameState->BlueOffset, GameState->GreenOffset);
}