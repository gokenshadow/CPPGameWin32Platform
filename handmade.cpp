#include "handmade.h"

void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz){
	int16 ToneVolume = 3000;
	//int ToneHz = 256;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	
	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex=0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
		//TODO(casey): Draw this out for people
		#if 0
		real32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		#else
		int16 SampleValue = 0;
		#endif
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		
		#if 0
		GameState->tSine += 2.0f*Pi32*1.0f/ (real32)WavePeriod;
		if(GameState->tSine > 2.0f*Pi32) {
			GameState->tSine -= 2.0f*Pi32;
		}
		#endif
	}
}

internal int32 RoundReal32ToInt32(real32 Real32) {
	int Result = (int32)(Real32 + 0.5f);
	// TODO(casey): Intrinsic????
	return(Result);
}

internal void DrawRectangle(game_offscreen_buffer *Buffer, 
							real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY,
							uint32 Color){
	// TODO(casey): Floating point color tomorrow!!!!
	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MaxX = RoundReal32ToInt32(RealMaxX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxY = RoundReal32ToInt32(RealMaxY);

	if(MinX < 0) {
		MinX = 0;
	}
	if(MinY < 0) {
		MinY = 0;
	}
	if(MaxX > Buffer->Width) {
		MaxX = Buffer->Width;
	}
	if(MaxY > Buffer->Height) {
		MaxY = Buffer->Height;
	}
	
	uint8 *EndBuffer = (uint8 *)Buffer->Memory  + Buffer->Pitch*Buffer->Height;
	
	//uint32 Color = 0xFFFFFF00;
	uint8 *Row = (uint8 *)Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch;
	for (int Y = MinY; Y < MaxY; ++Y) {
		uint32 *Pixel = (uint32 *)Row;
		for (int X = MinX; X < MaxX ; ++X) {
			*Pixel++ = Color;
		}
		
		Row += Buffer->Pitch;				
	}		
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons));
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized) {
		Memory->IsInitialized = true;
	}
	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex) {
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if(Controller->IsAnalog){
			// NOTE:(casey) Use analoy movement tuning
		} else {
			// NOTE:(casey) Use digital movement tuning			
		}
		
		DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 0x00FF00FF);
		DrawRectangle(Buffer, 1.f, 5.f, 30.0f, 30.0f, 0x0000FFFF);
		
	}	
	
}

extern "C" void GameGetSoundSamples(thread_context *Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer) {
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(GameState, SoundBuffer, 400);
}



/*
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
}*/
