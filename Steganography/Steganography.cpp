// Steganography.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include <stdlib.h>
#include <stdio.h>
#include "Steganography.h"
#include "lodepng.h"
#include <iostream>
#include <bitset>
#include <fstream>

std::vector<unsigned char> _image;
unsigned int _width;
unsigned int _height;

int main(int argc, char** argv) {
	std::cout << getFileName(std::string(argv[2])) << "\n";

	if (strcmp(argv[1], "a") == 0) {

	}



	decodeOneStep(argv[1]);
	std::vector<unsigned char> dataToHide = readAllBytes(argv[2]);
	unsigned char* pixel = _image.data();

	
	writeLengthHeader((unsigned int)dataToHide.size(), pixel);

	readLengthHeader(pixel);
	

	return 0;
}

void decodeOneStep(const char* filename) {
	std::vector<unsigned char> image; //the raw pixels
	unsigned width, height;

	//decode
	unsigned error = lodepng::decode(image, width, height, filename);

	//if there's an error, display it
	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
	_image = image;
	_width = width;
	_height = height;
}

void encodeOneStep(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height) {
	//Encode the image
	unsigned error = lodepng::encode(filename, image, width, height);

	//if there's an error, display it
	if (error) std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
}

std::vector<unsigned char> readAllBytes(std::string fileName) {
	//open file
	std::basic_ifstream<unsigned char> infile(fileName);
	std::vector<unsigned char> buffer;

	//get length of file
	infile.seekg(0, infile.end);
	size_t length = infile.tellg();
	infile.seekg(0, infile.beg);

	//read file
	if (length > 0) {
		buffer.resize(length);
		infile.read(&buffer[0], length);
	}

	return buffer;
}

std::string getFileName(std::string filename) {
	//Get index of last slash in path.
	const size_t last_slash_idx = filename.find_last_of("\\/");
	
	//Check if slash is the last character in the path.
	if (std::string::npos != last_slash_idx)
	{
		//Erase everything before the last slash, including last slash.
		filename.erase(0, last_slash_idx + 1);
	}
	return filename;
}

unsigned char setLastBit(unsigned char byte, int bit) {

	//Create a mask of the original byte with 0xFE (0x11111110 or 254) via bitwise AND
	int maskedOriginal = (byte & 0xFE);

	//Create modified byte from the previously created mask and the bit that should be set via bitwise OR
	unsigned char modifiedByte = (maskedOriginal | bit);

	return modifiedByte;
}

int getLastBit(unsigned char byte) {

	//Read LSB (Least significant bit) via bitwise AND with 0x1 (0x00000001 or 1)
	int bit = (byte & 0x1);

	return bit;
}

void writeLengthHeader(long length, unsigned char *pixel) {

	//Create 32 bit bitset to contain header and initialize it with the value of length (how many bytes of data will be hidden in the image)
	std::bitset<32> header(length);

	std::cout << header.to_ulong() << " write header\n";

	//create reversed index
	int reversedIndex;
	
	for (int i = 0; i < 32; i++) {
		reversedIndex = 31 - i;

		//Overwrite the byte at position i (0-31) of the decoded pixel data with a modified byte. 
		//The LSB of the modified byte is set to the bit value of header[reversedIndex] as the header is written to file starting with the MSB and the bitset starts with the LSB.
		pixel[i] = setLastBit(pixel[i], header[reversedIndex]);
	}
}

int readLengthHeader(unsigned char *pixel) {

	//Create 32 bit bitset to contain the header thats read from file.
	std::bitset<32> headerBits;

	//create reversed index
	int reversedIndex;

	for (int i = 0; i < 32; i++) {
		reversedIndex = 31 - i;

		//Set the bits of the header to the LSB of the read byte.
		//The bits are written to the bitset in reverse order as the bits in the bitset start with the LSB and the bits read from file start with MSB.
		headerBits[reversedIndex] = getLastBit(pixel[i]);
	}

	std::cout << headerBits.to_ulong() << " read header\n";

	//Return the header bitset as int
	return headerBits.to_ulong();
}

void hideDataInImage() {

}

std::vector<unsigned char> extractDataFromImage(int length, unsigned char *pixel) {
	const int offset = 2080;
	std::vector<unsigned char> data(length);

	std::bitset<8> byte;

	for (int i = 0; i < length; i++) {

		for (int ii = 8; ii >= 0; ii--) {
			byte[ii] = getLastBit(pixel[8 - ii + offset]);
		}

		data[i] = (unsigned char)byte.to_ulong();
		byte.reset();
	}
	return data;
}