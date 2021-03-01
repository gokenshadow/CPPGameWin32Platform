#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


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


#define DATA_OFFSET_OFFSET 0x000A
#define WIDTH_OFFSET 0x0012
#define HEIGHT_OFFSET 0x0016
#define BITS_PER_PIXEL_OFFSET 0x001C
#define HEADER_SIZE 14
#define INFO_HEADER_SIZE 40
#define NO_COMPRESION 0
#define MAX_NUMBER_OF_COLORS 0
#define ALL_COLORS_REQUIRED 0
#define BACKGROUND_COLOR 0xFFFFFFFF

void blah(){
	std::cout << "blah\n";
}

extern "C" void PrintSomethingCool() {
	std::cout << "This is from a the ScreenTestGameCode DLL!" << "\n";
}

struct bmp_pixel {
	uint8 blue;
	uint8 green;
	uint8 red;
};
void b () {
}

extern "C" void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_state *GameState,
									game_controller_input *Controller, game_sound_output_buffer *SoundBuffer,
									game_memory *Memory) {
	static byte *pixels;
	static int ImageX = 0;
	static int ImageY = 0;
	static bmp_image_data TestImageData;

	if(!TestImageData.Width) {
		TestImageData = Memory->GetBmpImageData("image.bmp");
	}

	if(GameState->IsInitialized == false) {
		std::cout << "ImageX:" << ImageX << "\n";
		GameState->ToneHz = 256;
		GameState->tSine = 0.0f;
		GameState->XSpeed = 2;
		GameState->YSpeed = 2;
		GameState->IsInitialized = true;
		GameState->ToneLevel = 256;
		
		bmp_image_data Tests[100]; 	
		
		
		//Memory->ClearBmpImageData(TestImageData);		

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
			
			//Pixel in Memory: BB GG RR xx
			//LITTLE ENDIAN ARCHITECTURE
			//0x xxBBGGRR

		   uint8 Blue = (X + GameState->XOffset);
		   uint8 Green = (Y + GameState->YOffset);
		   uint8 Red = (X + GameState->XOffset);


			//Memory:     BB GG RR xx
			//Register:   xx RR GG BB


			*Pixel++ = ((Red << 16) | (Green << 16) | Blue);
		}
		Row += Buffer->Pitch;
	}

	// DO STUFF BASED ON USER INPUT
	// ---------------
	// ---------------
	//GameState->XOffset+=1;
	//GameState->YOffset+=1;

	if(Controller->IsConnected) {
		if(Controller->IsAnalog) {
			//GameState->XOffset += (int)(16.0f*(Controller->StickAverageX));
			//GameState->YOffset -= (int)(16.0f*(Controller->StickAverageY));
			ImageX += (int)(16.0f*(Controller->StickAverageX));
			ImageY -= (int)(16.0f*(Controller->StickAverageY));
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

	// DRAW A BMP IMAGE ON THE SCREEN
	// ---------------
	// ---------------
	int ImageEndX = (ImageX + TestImageData.Width);
	int ImageEndY = (ImageY + TestImageData.Height);
	/*
	if(ImageX > (int)(Buffer->Width - TestImageData.Width)) {
		ImageX = Buffer->Width - TestImageData.Width;
	}
	if(ImageX < 0 ) {
		ImageX = 0;
	}
	if(ImageY < 0) {
		ImageY = 0;
	}
	if(ImageY > Buffer->Height - TestImageData.Height) {
		ImageY = Buffer->Height - TestImageData.Height;
	}
	*/
	
	//std::cout << "ImageX:" << Buffer->Width - TestImageData.Width << "\n";
	//std::cout << "ImageX:" << ImageX << "\n";
	//std::cout << "ImageY:" << ImageY << "\n";
			
	#if 0
	#endif
	uint8 *BmpRow = (uint8*)Buffer->Memory;
	uint32 ImagePitch = TestImageData.Width*TestImageData.BytesPerPixel;
	uint32 DIBPixel;
	uint8 *DataImageRow = (uint8*)TestImageData.ImageData;
	BmpRow += ImageX*Buffer->BytesPerPixel;
	BmpRow += ImageY*Buffer->Pitch;
	b();
	for(int Y = 0; Y<TestImageData.Height; ++Y){
		bmp_pixel *BMPPixel = (bmp_pixel*)DataImageRow;
		uint32 *Pixel = (uint32 *)BmpRow;
		for(int X = 0; X < TestImageData.Width; ++X) {
			//Convert the BMP Pixel (24bit) into a DIB Pixel (32 bit)
			DIBPixel = (0xFF << 24) |(BMPPixel->red << 16) | (BMPPixel->green<<8) | (BMPPixel->blue);
			// If the current Pixel we're on is our chosen background color
			// don't draw it, so that part of the image can be transparent
			if(DIBPixel!=BACKGROUND_COLOR 
				&& X+ImageX < (int)Buffer->Width
				&& X+ImageX > 0
				&& Y+ImageY < (int)Buffer->Height
				&& Y+ImageY > 0){
				*Pixel = DIBPixel;		
			}
			*BMPPixel++;
			*Pixel++;
		}
		BmpRow += Buffer->Pitch;
		DataImageRow += ImagePitch;
	}
	b();

	// Move the weird gradient from Right to left
	//GameState->XOffset+=GameState->XSpeed;
	//GameState->YOffset+=GameState->YSpeed;
	//blah();
}

extern "C" void GameUpdateSound(game_sound_output_buffer *SoundBuffer) {
	static real32 tSine=0;
}

