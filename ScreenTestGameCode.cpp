#include <stdint.h>
#include <stdio.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#define Pi32 3.14159265359f

#include <iostream>
#include <windows.h>
#include <math.h>
#include "ScreenTest.h"
#include "debugbreak.h"


void blah(){
	std::cout << "blah\n";
}

extern "C" void PrintSomethingCool() {
	std::cout << "This is from a the ScreenTestGameCode DLL!" << "\n";
}

extern "C" void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_state *GameState,
									game_controller_input *Controller, game_sound_output_buffer *SoundBuffer) {
	//debug_break();
	if(GameState->IsInitialized == false) {
		GameState->ToneHz = 256;
		GameState->tSine = 0.0f;
		GameState->XSpeed = 2;
		GameState->YSpeed = 2;
		GameState->IsInitialized = true;
		GameState->ToneLevel = 256;
	}
	
	// GENERATE OUR SOUND
	// ---------------
	// ---------------
	
	int16 ToneVolume = 3000;
	
	int WavePeriod = SoundBuffer->SamplesPerSecond/GameState->ToneHz;
	
	int16 *SampleOut = SoundBuffer->Samples;

	for(int SampleIndex=0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
		real32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
	
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		
		GameState->tSine += ((2.0f*Pi32) / (real32)WavePeriod)*1.0f;
		
		if(GameState->tSine > 2.0f*Pi32) {
			GameState->tSine -= 2.0f*Pi32;
		}
	}
	
	// RENDER A WEIRD GRADIENT THING
	// ---------------
	// ---------------
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
		   uint8 Blue = (X + GameState->XOffset);
		   uint8 Green = (Y + GameState->YOffset);
		   uint8 Red = (X + GameState->XOffset);
			
		   /*
			Memory:     BB GG RR xx
			Register:   xx RR GG BB
		   */

			*Pixel++ = ((Red << 16) | (Green << 16) | Blue);
		}
		Row += Buffer->Pitch;
	}
	
	//GameState->XOffset+=5;
	//GameState->YOffset+=5;
	
	if(Controller->IsConnected) {
		if(Controller->IsAnalog) {
			GameState->XOffset += (int)(16.0f*(Controller->StickAverageX));
			GameState->YOffset -= (int)(16.0f*(Controller->StickAverageY));
			GameState->ToneHz = GameState->ToneLevel + (int)(128.0f*(Controller->StickAverageY));
		}
		if(Controller->MoveUp) {
			GameState->YOffset -= 2;
		}
		if(Controller->MoveDown) {
			GameState->YOffset += 2;
		}
		if(Controller->MoveLeft) {
			GameState->XOffset -= 2;
		}
		if(Controller->MoveRight) {
			GameState->XOffset += 2;
		}
	}
	


	// Move the weird gradient from Right to left
	//GameState->XOffset+=GameState->XSpeed;
	//GameState->YOffset+=GameState->YSpeed;
	//blah();
}

extern "C" void GameUpdateSound(game_sound_output_buffer *SoundBuffer) {
	static real32 tSine=0;
}

