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

	
	writeLengthHeader(dataToHide.size(), pixel);

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

unsigned char setLastBit(unsigned char byte, int bit) {

	int maskedOriginal = (byte & 0xFE);
	unsigned char modifiedByte = (maskedOriginal | bit);

	return modifiedByte;
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
	const size_t last_slash_idx = filename.find_last_of("\\/");
	if (std::string::npos != last_slash_idx)
	{
		filename.erase(0, last_slash_idx + 1);
	}
	return filename;
}

int getLastBit(unsigned char byte) {

	int bit = (byte & 0x1);

	return bit;
}

void writeLengthHeader(long length, unsigned char *pixel) {
	std::bitset<32> header(length);

	std::cout << header.to_ulong() << " write header\n";
	int reversedIndex;
	
	for (int i = 0; i < 32; i++) {
		reversedIndex = 31 - i;
		pixel[i] = setLastBit(pixel[i], header[reversedIndex]);
	}
}

int readLengthHeader(unsigned char *pixel) {

	std::bitset<32> headerBits;

	for (int i = 31; i >= 0; i--) {
		headerBits[i] = getLastBit(pixel[31-i]);
	}

	std::cout << headerBits.to_ulong() << " read header\n";

	return headerBits.to_ulong();
}

void writeWidthHeader(long length, unsigned char* pixel) {
	int offset = 32;
	std::bitset<16> header(length);

	std::cout << header.to_ulong() << " write width header\n";
	int reversedIndex;

	for (int i = 0; i < 16; i++) {
		reversedIndex = 15 - i;
		pixel[i + offset] = setLastBit(pixel[i + offset], header[reversedIndex]);
	}
}

int readWidthHeader(unsigned char* pixel) {
	int offset = 32;
	std::bitset<16> headerBits;

	for (int i = 15; i >= 0; i--) {
		headerBits[i] = getLastBit(pixel[offset + 15 - i]);
	}

	std::cout << headerBits.to_ulong() << " read header\n";

	return headerBits.to_ulong();
}

void writeHeightHeader(long length, unsigned char* pixel) {
	int offset = 48;
	std::bitset<16> header(length);

	std::cout << header.to_ulong() << " write height header\n";
	int reversedIndex;

	for (int i = 0; i < 16; i++) {
		reversedIndex = 15 - i;
		pixel[i + offset] = setLastBit(pixel[i + offset], header[reversedIndex]);
	}
}

int readHeightHeader(unsigned char* pixel) {
	int offset = 48;
	std::bitset<16> headerBits;

	for (int i = 15; i >= 0; i--) {
		headerBits[i] = getLastBit(pixel[offset + 15 - i]);
	}

	std::cout << headerBits.to_ulong() << " read header\n";

	return headerBits.to_ulong();
}

void hideDataInImage() {

}

void extractDataFromImage() {

}