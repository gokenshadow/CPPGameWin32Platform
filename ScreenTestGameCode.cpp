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
									game_controller_input *Controller, game_sound_output_buffer *SoundBuffer) {
	static byte *pixels;
	static int32 width;
	static int32 height;
	static int32 bytesPerPixel;
	static int32 Width;
	static int32 Height;
	static int32 BytesPerPixel;
	static int32 newSize;
	static void *imageMemory;
	static void *NewImageData;
	static uint8 *ImageData;
	static uint32 ImageX =100;
	static uint32 ImageY =0;

	if(GameState->IsInitialized == false) {
		GameState->ToneHz = 256;
		GameState->tSine = 0.0f;
		GameState->XSpeed = 2;
		GameState->YSpeed = 2;
		GameState->IsInitialized = true;
		GameState->ToneLevel = 256;
		
		// LOAD A BMP IMAGE INTO MEMORY
		// ---------------
		// ---------------
		FILE *filePointer = fopen("image.bmp", "rb");
		// get file size
		uint32 FileSize;
		fseek(filePointer, 2, SEEK_SET);
		fread(&FileSize, sizeof(uint32), 1, filePointer);
		std::cout << "FileSize:" << FileSize << "\n";

		// get the data offset
		int32 DataOffset;
		fseek(filePointer, DATA_OFFSET_OFFSET, SEEK_SET);
		fread(&DataOffset, sizeof(uint32), 1, filePointer);
		fseek(filePointer, DataOffset, SEEK_SET);
		std::cout << "DataOffset:" << DataOffset << "\n";

		// get width /height
		fseek(filePointer, WIDTH_OFFSET, SEEK_SET);
		fread(&Width, 4, 1, filePointer);
		fseek(filePointer, HEIGHT_OFFSET, SEEK_SET);
		fread(&Height, 4, 1, filePointer);
		std::cout << "" << "Width:" << Width << " Height:" << Height << "\n";

		// get the bytesperpixel;
		int16 BitsPerPixel;
		fseek(filePointer, BITS_PER_PIXEL_OFFSET, SEEK_SET);
		fread(&BitsPerPixel, 2, 1, filePointer);
		BytesPerPixel = ((int32)BitsPerPixel) / 8;
		std::cout << "BytesPerPixel:" << BytesPerPixel << "\n";

		// get the padded width
		uint32 PaddedWidth = (uint32)(ceil((float)Width / 4.0)) * 4;
		std::cout << "PaddedWidth:" << PaddedWidth << "\n";

		// get the actual size of the data
		uint32 ActualDataSize = PaddedWidth*Height*BytesPerPixel;

		// get the size of the Data we want
		uint32 DataSize = Width*Height*BytesPerPixel;
		std::cout << "DataSize:" << DataSize << "\n";
		
		// get the size of the final Data
		uint32 FinalDataSize = Width*Height*4;

		// allocate space for the data we want
		ImageData = (uint8*)malloc(DataSize);
		
		// Write the image data to the allocated 
		
		// Grab the last row of the memory we allocated
		uint8 *CurrentRow = ImageData+((Height-1)*Width*BytesPerPixel);
		for (int i = 0; i < Height; i++)
		{
			fseek(filePointer, DataOffset+(i*PaddedWidth*BytesPerPixel), SEEK_SET);
			fread(CurrentRow, 1, Width*BytesPerPixel, filePointer);
			CurrentRow -= Width*BytesPerPixel;
		}

	}


	// GENERATE OUR SOUND
	// ---------------
	// ---------------

	int16 ToneVolume = 0;

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

	// DRAW A BMP IMAGE ON THE SCREEN
	// ---------------
	// ---------------
	if(ImageX<Buffer->Width*BytesPerPixel) {
		ImageX = 0;
	}
	if(ImageX>Buffer->Width*BytesPerPixel) {
		ImageX = Buffer->Width*BytesPerPixel;
	}
	if(ImageY<Buffer->Width*BytesPerPixel) {
		ImageY = 0;
	}
	if(ImageY>Buffer->Width*BytesPerPixel) {
		ImageY = Buffer->Width*BytesPerPixel;
	}
	
	#if 0
	#endif
	uint32 BackgroundColor = 0xFFFFFFFF;
	uint8 *BmpRow = (uint8*)Buffer->Memory;
	uint32 ImagePitch = Width*BytesPerPixel;
	uint32 DIBPixel;
	uint8 *DataImageRow = (uint8*)ImageData;
	for(int Y = 0; Y<Height; ++Y){
		bmp_pixel *BMPPixel = (bmp_pixel*)DataImageRow;
		uint32 *Pixel = (uint32 *)BmpRow;
		for(int X = 0; X < Width; ++X) {
			//Convert the BMP Pixel (24bit) into a DIB Pixel (32 bit)
			DIBPixel = (0xFF << 24) |(BMPPixel->red << 16) | (BMPPixel->green<<8) | (BMPPixel->blue);
			// If the current Pixel we're on is our chosen background color
			// don't draw it, so that part of the image can be transparent
			if(DIBPixel!=BackgroundColor){
				*Pixel = DIBPixel;				
			}
			*BMPPixel++;
			*Pixel++;
		}
		BmpRow += Buffer->Pitch;
		DataImageRow += ImagePitch;
	}

	//GameState->XOffset+=80;
	//GameState->YOffset+=80;

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

