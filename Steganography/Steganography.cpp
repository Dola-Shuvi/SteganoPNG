#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include "Steganography.h"
#include "lodepng.h"
#include <iostream>
#include <bitset>
#include <fstream>
#include <sstream>
#include <filesystem>

std::vector<unsigned char> _image;
unsigned int _width;
unsigned int _height;

int main(int argc, char** argv) {

	if ((argc != 3 && argc != 4 || (strcmp(argv[1], "h") == 0) || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
		printHelp();
		exit(EXIT_SUCCESS);
	}

	if (strcmp(argv[1], "a") == 0) {

		if (!(std::filesystem::exists(argv[2]) && std::filesystem::exists(argv[3]))) {
			std::cout << "One or more specified files do not exist\n";
			exit(EXIT_FAILURE);
		}

		decodeOneStep(argv[2]);
		std::vector<unsigned char> dataToHide = readAllBytes(argv[3]);
		unsigned char* pixel = _image.data();

		writeLengthHeader((unsigned int)dataToHide.size(), pixel);
		writeFilenameHeader(getFileName(std::string(argv[3])), pixel);
		hideDataInImage(dataToHide, pixel);

		encodeOneStep(argv[2], _image, _width, _height);

		exit(EXIT_SUCCESS);
	}
	else if (strcmp(argv[1], "x") == 0){

		if (!std::filesystem::exists(argv[2])) {
			std::cout << "One or more specified files do not exist\n";
			exit(EXIT_FAILURE);
		}

		decodeOneStep(argv[2]);
		unsigned char* pixel = _image.data();

		int length = readLengthHeader(pixel);
		std::string filename = readFilenameHeader(pixel);
		std::vector<unsigned char> extractedData = extractDataFromImage(length, pixel);
		writeAllBytes(filename, extractedData);

	}
	else if (strcmp(argv[1], "t") == 0) {

		if (!(std::filesystem::exists(argv[2]) && std::filesystem::exists(argv[3]))){
			std::cout << "One or more specified files do not exist\n";
			exit(EXIT_FAILURE);
		}

		decodeOneStep(argv[2]);
		validateStorageSpace(argv[2], argv[3]);
		exit(EXIT_SUCCESS);
	}
	else {
		printHelp();
		exit(EXIT_FAILURE);
	}

	return 0;
}

void decodeOneStep(const char* filename) {
	std::vector<unsigned char> image; //the raw pixels
	unsigned width, height;

	//decode
	unsigned error = lodepng::decode(image, width, height, filename);

	//if there's an error, display it
	if (error) {
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		std::cout << "The image you provided is corrupted or not supported. Please provide a PNG file.\n";
		exit(EXIT_FAILURE);
	}

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
	//Open file
	std::ifstream infile(fileName, std::ios::in | std::ios::binary);
	std::vector<unsigned char> buffer;

	//Get length of file
	infile.seekg(0, infile.end);
	size_t length = infile.tellg();
	infile.seekg(0, infile.beg);

	//Read file
	if (length > 0) {
		buffer.resize(length);
		infile.read((char*)&buffer[0], length);
	}

	//Close instream
	infile.close();

	std::ofstream of;
	of.open("debugReadAllBytes.txt", std::ios::out);

	std::cout << buffer.size() << "\n";
	for (int i = 0; i < buffer.size(); i++) {
		of << buffer[i];
	}

	of.close();

	return buffer;
}

void writeAllBytes(std::string fileName, std::vector<unsigned char> data) {

	std::ofstream debugof;
	debugof.open("debugWriteAllBytes.txt", std::ios::out);

	std::cout << data.size() << "\n";
	for (int i = 0; i < data.size(); i++) {
		debugof << data[i];
	}

	debugof.close();
	
	std::ofstream of;
	of.open(fileName, std::ios::out | std::ios::binary);
	of.write((char*)&data[0], data.size());
	of.close();

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

std::string TextToBinaryString(std::string words) {
	std::string binaryString = "";

	//Loop through chars in string, put them in a bitset and return the bitset for each char as a string.
	for (char& _char : words) {
		binaryString += std::bitset<8>(_char).to_string();
	}
	return binaryString;
}

void printHelp() {
	std::cout << "Syntax: Steganography <command> <image.png> [data.xxx]" << "\n\n"
		<< "Commands:" << "\n"
		<< "\ta" << "\t" << "Hide provided file in image" << "\n"
		<< "\tx" << "\t" << "Extract file hidden in image" << "\n"
		<< "\tt" << "\t" << "Verify if the image contains enough pixels for storage" << "\n"
		<< "\th" << "\t" << "Show this help screen" << "\n\n"
		<< "Examples:" << "\n"
		<< "\tSteganography a image.png FileYouWantToHide.xyz\t to hide \"FileYouWantToHide.xyz\" inside image.png" << "\n"
		<< "\tSteganography x image.png\t\t\t to extract the file hidden in image.png" << "\n"
		<< "\tSteganography v image.png FileYouWantToHide.xyz\t to verify that the image contains enough pixels for storage" << "\n\n"
		<< "Use this software at your OWN risk"
		;
}

bool validateStorageSpace(char* imageFile, char* dataFile) {

	bool result = false;

	int subpixelcount = _width * _height * 4;
	int dataLength = 8 * readAllBytes(dataFile).size();
	if ((subpixelcount - 32 - 2048) > dataLength) {
		std::cout << "The file " << getFileName(imageFile) << " contains enough pixels to hide all data of " << getFileName(dataFile) << " .\n";
		result = true;
	}
	else {
		std::cout << "The file " << getFileName(imageFile) << " does not contain enough pixels to hide all data of " << getFileName(dataFile) << " .\n";
		exit(EXIT_FAILURE);
	}

	return result;
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

	//create reversed index
	int reversedIndex;
	
	for (int i = 0; i < 32; i++) {
		reversedIndex = 31 - i;

		//Overwrite the byte at position i (0-31) of the decoded subpixel data with a modified byte. 
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

	//Return the header bitset as int
	return headerBits.to_ulong();
}

void writeFilenameHeader(std::string fileName, unsigned char *pixel) {

	//Create 2048 bit bitset to contain filename header and initialize it with the value of filename.
	std::bitset<2048> header(TextToBinaryString(fileName));

	//Create offset to not overwrite length header.
	const int offset = 32;

	//create reversed index.
	int reversedIndex;
	
	for (int i = 0; i < 2048; i++) {
		reversedIndex = 2047 - i;

		//Overwrite the byte at position i (32-2079) of the decoded subpixel data with a modified byte. 
		//The LSB of the modified byte is set to the bit value of header[reversedIndex] as the header is written to file starting with the MSB and the bitset starts with the LSB.
		pixel[i + offset] = setLastBit(pixel[i + offset], header[reversedIndex]);
	}
}

std::string readFilenameHeader(unsigned char *pixel) {

	//Create 2048 bit bitset to contain the header thats read from file.
	std::bitset<2048> headerBits;

	//Create offset to not read length header.
	const int offset = 32;

	//create reversed index
	int reversedIndex;

	for (int i = 0; i < 2048; i++) {
		reversedIndex = 2047 - i;

		//Set the bits of the header to the LSB of the read byte.
		//The bits are written to the bitset in reverse order as the bits in the bitset start with the LSB and the bits read from file start with MSB.
		headerBits[reversedIndex] = getLastBit(pixel[i+offset]);
	}

	//Convert binary to ascii
	std::stringstream sstream(headerBits.to_string());
	std::string output;
	while (sstream.good())
	{
		std::bitset<8> bits;
		sstream >> bits;
		char c = char(bits.to_ulong());
		output += c;
	}
	//Remove all NUL chars from output
	output.erase(std::remove(output.begin(), output.end(), '\0'), output.end());

	//Return the header bitset as int
	return output;
}

void hideDataInImage(std::vector<unsigned char> data, unsigned char *pixel) {

	//Offset in subpixels to not overwrite header data.
	const int offset = 2080;

	//Create offset to track progress.
	int progressOffset;

	//Create reversed index.
	int reversedIndex;

	//Create 8 bit buffer to store bits of each byte.
	std::bitset<8> buffer;

	for (int i = 0; i < data.size(); i++) {
		buffer = data[i];
		progressOffset = i * 8;

		for (int ii = 0; ii < 8; ii++) {
			reversedIndex = 7 - ii;
			pixel[ii + progressOffset + offset] = setLastBit(pixel[ii + progressOffset + offset], buffer[reversedIndex]);
		}



	}

}

std::vector<unsigned char> extractDataFromImage(int length, unsigned char *pixel) {

	//Offset in subpixels to not read header data.
	const int offset = 2080;

	//Initialize vector to hold extracted data.
	std::vector<unsigned char> data(length);

	//Create 8 bit bitset to store and convert the read bits.
	std::bitset<8> byte;

	//Counter to track which subpixel to read next.
	int progressOffset;

	//Loop through as many bytes as length tells us to.
	for (int i = 0; i < length; i++) {
		progressOffset = i * 8;
		//For each byte of extracted data loop through 8 subpixels (2 full pixels).
		for (int ii = 7; ii >= 0; ii--) {
			//Set the bits of the header to the LSB of the read byte.
			//The bits are written to the bitset in reverse order as the bits in the bitset start with the LSB and the bits read from file start with MSB.
			byte[ii] = getLastBit(pixel[7 - ii + offset + progressOffset]);
		}

		data[i] = (unsigned char)byte.to_ulong();
	}

	std::cout << data.size() << "\n";

	return data;
}