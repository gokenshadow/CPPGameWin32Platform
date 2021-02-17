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

#include <iostream>
#include <windows.h>
#include "ScreenTest.h"
#include "debugbreak.h"


void blah(){
	std::cout << "blah\n";
}

extern "C" void PrintSomethingCool() {
	std::cout << "This is from a the ScreenTestGameCode DLL!" << "\n";
}



extern "C" void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_state *GameState) {
	//debug_break();
	if(GameState->IsInitialized == false) {
		GameState->ToneHz = 512;
		GameState->tSine = 0.0f;
		GameState->XSpeed = 2;
		GameState->YSpeed = 2;
		GameState->IsInitialized = true;
	}
	//Assert (0);
	//static int XOffset = 0;
	//static int YOffset = 0;
	
	GameState->XSpeed = 1;
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

	// Move the weird gradient from Right to left
	GameState->XOffset+=GameState->XSpeed;
	GameState->YOffset+=GameState->YSpeed;
	//blah();
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {

    return TRUE;
}
