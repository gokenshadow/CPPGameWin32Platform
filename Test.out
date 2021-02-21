#include "handmade.h"


internal void RenderWeirdGradient (game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
    uint8 *Row = (uint8*)Buffer->Memory;
    for(int Y = 0; Y<Buffer->Height; ++Y){
        uint32 *Pixel = (uint32 *)Row;
        for(int X = 0; X < Buffer->Width;
        ++X) {
            
                //Pixel in Memory: BB GG RR xx
                //LITTLE ENDIAN ARCHITECTURE

                //0x xxBBGGRR 
            
           uint8 Blue = (uint8)(X + XOffset);
           uint8 Green = (uint8)(Y + YOffset);
		   uint8 Red = 	(uint8)(X + XOffset);

           
            //Memory:     BB GG RR xx
            //Register:   xx RR GG BB
           

            *Pixel++ = ((Red << 16) | (Green << 16) | Blue);
        }
        Row += Buffer->Pitch;
    }
}

internal void RenderPlayer(game_offscreen_buffer *Buffer, int PlayerX, int PlayerY){
	uint8 *EndBuffer = (uint8 *)Buffer->Memory  + Buffer->Pitch*Buffer->Height;
	uint32 Color = 0xFFFFFF00;
	int Top = PlayerY;
	int Bottom = PlayerY + 10;
	for (int X = PlayerX; X < PlayerX + 10; ++X) {
		
		uint8 *Pixel = (uint8 *)Buffer->Memory + X*Buffer->BytesPerPixel + Top*Buffer->Pitch;
		for (int Y = Top; Y < Bottom; ++Y) {
			if((Pixel >= Buffer->Memory)&&(Pixel + 4) <= EndBuffer) {
				*(uint32 *)Pixel = Color;
			}
			
			Pixel += Buffer->Pitch;				
		}		
	}
}

void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz){
	int16 ToneVolume = 3000;
	//int ToneHz = 256;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	
	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex=0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
		//TODO(casey): Draw this out for people
		#if HANDMADE_WIN32
		real32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		#else
		int16 SampleValue = 0;
		#endif
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
		
		debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Thread, Filename);
		if(File.Contents) {
			Memory->DEBUGPlatformWriteEntireFile(Thread, "test.out", File.ContentSize, File.Contents);
			Memory->DEBUGPlatformFreeFileMemory(Thread, File.Contents);			
		}
		
				
		GameState->ToneHz = 512;
		GameState->tSine = 0.0f;
		
		GameState->PlayerX = 100;
		GameState->PlayerY = 200;
		
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
			//std::cout << "PlayerX: " << GameState->PlayerX << "\n";
			// Use digital movement tuning
		}
		//Input.AButtonEndedDown;
		//Input.AButtonHalfTransitionCount;
		
		GameState->PlayerX += (int)(4.0f*(Controller->StickAverageX));
		GameState->PlayerY -= (int)(4.0f*(Controller->StickAverageY));
		if(GameState->tJump > 0) {
			GameState->PlayerY += (int)(10.0f*sinf(Pi32*GameState->tJump));
		}
		if(Controller->ActionDown.EndedDown) {
				GameState->tJump = 2.0f;		
		}
		GameState->tJump -= 0.033f;
	}
	
	
	

	RenderWeirdGradient (Buffer, GameState->BlueOffset, GameState->GreenOffset);
	RenderPlayer(Buffer, GameState->PlayerX, GameState->PlayerY);
	
	RenderPlayer(Buffer, Input->MouseX, Input->MouseY);		
	for(int ButtonIndex = 0; ButtonIndex < ArrayCount(Input->MouseButtons); ++ButtonIndex) {
		if(Input->MouseButtons[ButtonIndex].EndedDown) {
			RenderPlayer(Buffer, 10 + 20 * ButtonIndex, 10);
		}		
	}
}

extern "C" void GameGetSoundSamples(thread_context *Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer) {
	// TODO(casey): Allow sample offsets here for more robust platform options
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}

