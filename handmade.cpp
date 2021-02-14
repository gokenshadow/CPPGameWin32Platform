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

            *Pixel++ = ((Red << 16) | (Green << 16) | Blue);
        }
        Row += Buffer->Pitch;
    }
}

void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz){
	int16 ToneVolume = 3000;
	//int ToneHz = 256;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	
	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex=0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
		//TODO(casey): Draw this out for people
		real32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		GameState->tSine += 2.0f*Pi32*1.0f/ (real32)WavePeriod;
		if(GameState->tSine > 2.0f*Pi32) {
			GameState->tSine -= 2.0f*Pi32;
		}
	}
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons));
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized) {
		
		char *Filename = __FILE__;
		
		debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Filename);
		if(File.Contents) {
			Memory->DEBUGPlatformWriteEntireFile("test.out", File.ContentSize, File.Contents);
			Memory->DEBUGPlatformFreeFileMemory(File.Contents);			
		}
		
				
		GameState->ToneHz = 512;
		GameState->tSine = 0.0f;
		
		// TODO(casey): This may be more appropriate to do in the platform layer 
		Memory->IsInitialized = true;
	}
	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex) {
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if(Controller->IsAnalog){
			// NOTE:(casey) Use analoy movement tuning
			GameState->BlueOffset += (int)(4.0f*(Controller->StickAverageX));
			GameState->ToneHz = 512 + (int)(128.0f*(Controller->StickAverageY));
		} else {
			if(Controller->MoveLeft.EndedDown) {
				GameState->BlueOffset -= 2;
			}
			if (Controller->MoveRight.EndedDown) {
				GameState->BlueOffset += 2;
			} 
			// Use digital movement tuning
		}
		
		//Input.AButtonEndedDown;
		//Input.AButtonHalfTransitionCount;
		if(Controller->ActionDown.EndedDown) {
			GameState->GreenOffset += 1;
		}		
	}
	
	
	

	RenderWeirdGradient (Buffer, GameState->BlueOffset, GameState->GreenOffset);
}

extern "C" void GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer) {
	// TODO(casey): Allow sample offsets here for more robust platform options
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}

#if HANDMADE_WIN32
#include "windows.h"
BOOL WINAPI DllMain(
  _In_ HINSTANCE hinstDLL,
  _In_ DWORD     fdwReason,
  _In_ LPVOID    lpvReserved
  ) {
	  return(true);
  }
#endif