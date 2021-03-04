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
#define BACKGROUND_COLOR 0x00000000

void blah(){
	std::cout << "blah\n";
}

extern "C" void PrintSomethingCool() {
	std::cout << "This is from a the ScreenTestGameCode DLL!" << "\n";
}

void b () {
}

void DrawImage (bmp_image_data ImageData, int ImageX, int ImageY, game_offscreen_buffer *Buffer) {
			//std::cout << "ImageH:" << ImageY + (int)ImageData.Height << "\n";
		if((Buffer->Height >= ImageY&&ImageY + (int)ImageData.Height >=0)
			&&(Buffer->Width >= ImageX&&ImageX + (int)ImageData.Width >=0)
			){
			int ImageStartX = ImageX;
			int ImageStartY = ImageY;
			
			// Calculate Y
			if(ImageStartY<0) {
				ImageStartY = 0;
			}
			//std::cout << "ImageStartY:" << ImageStartY << "\n";
			int ImageEndY = (ImageY + (int)ImageData.Height);
			if(ImageEndY > Buffer->Height-0){
				ImageEndY = Buffer->Height-0;
			}
			
			
			// Calculate X 
			if(ImageStartX<0) {
				ImageStartX = 0;
			}
			int ImageEndX = (ImageX + (int)ImageData.Width);
			if(ImageEndX > Buffer->Width-0){
				ImageEndX = Buffer->Width-0;
			}
			
			int StartDrawingX=0;
			if(ImageX<0) {
				StartDrawingX = 0 - ImageX;
			}
			int StartDrawingY=0;
			if(ImageY<0) {
				StartDrawingY = 0 - ImageY;
			}
		
			uint8 *BufferRow = (uint8*)Buffer->Memory;
			uint8 *BMPRow = (uint8*)ImageData.ImageData;
			uint32 ImagePitch = ImageData.Width*ImageData.BytesPerPixel;
			uint32 DIBPixel;
			BMPRow += StartDrawingX*ImageData.BytesPerPixel;
			BMPRow += StartDrawingY*ImagePitch;
			BufferRow += ImageStartX*Buffer->BytesPerPixel;
			BufferRow += ImageStartY*Buffer->Pitch;
			b();
			
			uint32 UseHeight = ImageData.Height;
			uint32 UseWidth = ImageData.Width;
			if(UseHeight>Buffer->Height) {
				UseHeight = Buffer->Height;
			}
			if(UseWidth>Buffer->Width) {
				UseWidth = Buffer->Width;
			}
			for(int Y = ImageStartY; Y<ImageEndY; ++Y){
				uint32 *BMPPixel = (uint32*)BMPRow;
				uint32 *Pixel = (uint32 *)BufferRow;
				for(int X = ImageStartX; X < ImageEndX; ++X) { //how much of row is drawn
					DIBPixel = *BMPPixel;
					if(DIBPixel!=BACKGROUND_COLOR ){
						*Pixel = DIBPixel;		
					}
					*BMPPixel++; // Go to next Pixel in BMP row
					*Pixel++;    // Go to next Pixel in SCREEN row
				}
				BMPRow += ImagePitch;      // Go to next row in BMP IMAGE
				BufferRow += Buffer->Pitch; // Go to next row on SCREEN
			}
			
		}
}

extern "C" void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_state *GameState,
									game_controller_input *Controller, game_sound_output_buffer *SoundBuffer,
									game_memory *Memory) {
	static byte *pixels;
	static int ImageX = 0;
	static int ImageY = 0;
	static bmp_image_data Image1;
	static bmp_image_data Image2;
	static bmp_image_data Image3;
	static bmp_image_data Image4;
	static bmp_image_data Image5;

	if(!Image1.Width) {
		Image1 = Memory->GetBmpImageData("171.bmp");
		Image2 = Memory->GetBmpImageData("171.bmp");
		/*Image3 = Memory->GetBmpImageData("171.bmp");
		Image4 = Memory->GetBmpImageData("171.bmp");
		Image5 = Memory->GetBmpImageData("171.bmp");*/
	}

	if(GameState->IsInitialized == false) {
		std::cout << "ImageX:" << ImageX << "\n";
		GameState->ToneHz = 256;
		GameState->tSine = 0.0f;
		GameState->XSpeed = 2;
		GameState->YSpeed = 2;
		GameState->IsInitialized = true;
		GameState->ToneLevel = 256;
		
		//Memory->ClearBmpImageData(Image1);		

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

	
	
	/*uint8 *Row = (uint8*)Buffer->Memory;
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
	}*/
	
	#if 0 
	#endif
	// Black screen
	uint32 *BlackBackgroundPixel = (uint32*)Buffer->Memory;
	for(int i = 0; i<(Buffer->Width*Buffer->Height); i++) {
		*BlackBackgroundPixel++ = 0;
	}

	// DO STUFF BASED ON USER INPUT
	// ---------------
	// ---------------
	//GameState->XOffset+=1;
	//GameState->YOffset+=1;
	//GameState->XOffset+=Controller->MouseDeltaX;
	//GameState->YOffset+=Controller->MouseDeltaY;
	ImageX-=Controller->MouseDeltaX;
	ImageY-=Controller->MouseDeltaY;
	if(Controller->IsConnected) {
		if(Controller->IsAnalog) {
			//GameState->XOffset += (int)(16.0f*(Controller->StickAverageX));
			//GameState->YOffset -= (int)(16.0f*(Controller->StickAverageY));
			ImageX += (int)(16.0f*(Controller->StickAverageX));
			ImageY -= (int)(16.0f*(Controller->StickAverageY));
			GameState->ToneHz = GameState->ToneLevel + (int)(128.0f*(Controller->StickAverageY));
		}
		if(Controller->MoveUp) {
			//GameState->YOffset += 8;
			ImageY += 4;
		}
		if(Controller->MoveDown) {
			//GameState->YOffset -= 8;
			ImageY -= 4;
		}
		if(Controller->MoveLeft) {
			//GameState->XOffset -= 2;
			ImageX -= 8;
		}
		if(Controller->MoveRight) {
			//GameState->XOffset += 2;
			ImageX += 8;
		}
	}

	// DRAW A BMP IMAGE ON THE SCREEN
	// ---------------
	// ---------------
	/*
	if(ImageX > (int)(Buffer->Width - Image1.Width)) {
		ImageX = Buffer->Width - Image1.Width;
	}
	if(ImageX < 0 ) {
		ImageX = 0;
	}
	if(ImageY < 0) {
		ImageY = 0;
	}
	if(ImageY > Buffer->Height - Image1.Height) {
		ImageY = Buffer->Height - Image1.Height;
	}
	*/
	
	//std::cout << "ImageX:" << Buffer->Width - Image1.Width << "\n";
	//std::cout << "ImageY:" << ImageY << "\n";
	int YExtra = 0;
	int XExtra = 0;
	DrawImage(Image1, ImageX+XExtra, ImageY+YExtra, Buffer);
	YExtra += Image1.Height/1.3;
	XExtra += Image1.Width;
	DrawImage(Image2, ImageX+XExtra, ImageY+YExtra, Buffer);
	YExtra += Image2.Height/1.3;
	XExtra += Image2.Width;
	/*
	DrawImage(Image3, ImageX+XExtra, ImageY+YExtra, Buffer);
	YExtra += Image3.Height/1.3;
	XExtra += Image3.Width;
	DrawImage(Image4, ImageX+XExtra, ImageY+YExtra, Buffer);
	YExtra += Image4.Height/1.3;
	XExtra += Image4.Width;
	DrawImage(Image5, ImageX+XExtra, ImageY+YExtra, Buffer);*/

	
	b();

	// Move the weird gradient from Right to left
	//GameState->XOffset+=GameState->XSpeed;
	//GameState->YOffset+=GameState->YSpeed;
	//blah();
}

extern "C" void GameUpdateSound(game_sound_output_buffer *SoundBuffer) {
	static real32 tSine=0;
}

