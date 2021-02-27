#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

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

#define DATA_OFFSET_OFFSET 0x000A
#define WIDTH_OFFSET 0x0012
#define HEIGHT_OFFSET 0x0016
#define BITS_PER_PIXEL_OFFSET 0x001C
#define HEADER_SIZE 14
#define INFO_HEADER_SIZE 40
#define NO_COMPRESION 0
#define MAX_NUMBER_OF_COLORS 0
#define ALL_COLORS_REQUIRED 0


struct bmp_pixel {
	uint8 red;
	uint8 green;
	uint8 blue;
};

void FakeFunction () {
}

int main(){
	char *text = (char*)malloc(200);
	
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

	// get width/height
	fseek(filePointer, WIDTH_OFFSET, SEEK_SET);
	int32 Width;
	fread(&Width, 4, 1, filePointer);
	int32 Height;
	fseek(filePointer, HEIGHT_OFFSET, SEEK_SET);
	fread(&Height, 4, 1, filePointer);
	std::cout << "" << "Width:" << Width << " Height:" << Height << "\n";
	
	// get the bytesperpixel;
	int32 BytesPerPixel;
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
	
	// allocate space for the data we want
	uint8 *ImageData = (uint8*)malloc(DataSize);
	
	// Write the image data to the allocated space starting from the bottom
	uint8 *CurrentRow = ImageData+((Height-1)*Width*BytesPerPixel);

	std::cout << "CurrentRow:" << std::hex << (*CurrentRow<<8>>8) << "\n";
	
	int PaddedRowSize = (int)(4 * ceil((float)(Width) / 4.0f))*(BytesPerPixel);
	int UnpaddedRowSize = (Width)*(BytesPerPixel);
	int TotalSize = UnpaddedRowSize*(Height);
	uint8 *Pixels = (uint8*)malloc(TotalSize);
	int i = 0;
	uint8 *CurrentRowPointer = Pixels+((Height-1)*UnpaddedRowSize);
	for (i = 0; i < Height; i++)
	{
		fseek(filePointer, DataOffset+(i*PaddedRowSize), SEEK_SET);
		fread(CurrentRowPointer, 1, UnpaddedRowSize, filePointer);
		CurrentRowPointer -= UnpaddedRowSize;
	}
	
	// Load the image data into that allocated space one row at a a time f
	/*for(int i = 0; i < Height*BytesPerPixel; i++) {
		fseek(filePointer, DataOffset+(i*PaddedWidth), SEEK_SET);
		fread(CurrentRow, BytesPerPixel, Width, filePointer);
		CurrentRow -= 1;		
	}*/
	
	// get the size of the Data after we turn it into 32 bit image data
	uint32 FinalDataSize = Width*Height*4;
	
	// allocate space for the 32 bit image data
	uint32 *FinalImageData = (uint32*)malloc(FinalDataSize);
	
	uint8 *NotherPixelPointer = (uint8*)Pixels;
	//uint32 *NewPixelPointer = (uint32*)FinalImageData;
	
	uint32 pix;
	uint32 stupid;
	for(int i=0; i<Width*Height*BytesPerPixel; i++) {
		pix = *NotherPixelPointer<<8;
		pix = pix>>8;
		std::cout << pix << " ";
		NotherPixelPointer++;
		/*stupid = (NotherPixelPointer->red << 24) | (NotherPixelPointer->green<<16) | (NotherPixelPointer->blue<<8);
		std::cout << "Stupid:" << std::hex << stupid << "\n";
		*NewPixelPointer = stupid;
		*NewPixelPointer++;
		*NotherPixelPointer++;*/
	}
	
	
	
	
	//char *blah2Pointer = blahPointer;
	//std::cout << "\n\n\n\nBlahPointer:"  << blah2Pointer  << "\n\n\n\n";
	
	char *letter = (char*)text;
	char sentence[27] =  "abcdefghijklmnopqrstuvwxyz";
	/*
	for(int i = 0; i<199; i++) {
		*letter++ = sentence[i%26];	
	}
	
	uint8 *bullshit = (uint8*)text;
	for(int i = 0; i<199; i++) {
		std::cout << "Letter:" << *bullshit++ << "\n";		
	}*/
	return 0;
}