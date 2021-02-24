#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <bitset>
#include <fstream>

#include "config.h"
#include <lodepng.h>

namespace SteganoPNG {

	void decodeOneStep(const char* filename, std::vector<unsigned char>* _image, unsigned int* _width, unsigned int* _height);

	void encodeOneStep(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height);

	unsigned char setLastBit(unsigned char byte, int bit);

	int getLastBit(unsigned char byte);

	std::vector<unsigned char> readAllBytes(std::string fileName);

	void writeAllBytes(std::string fileName, std::vector<unsigned char> data);

	std::string getFileName(std::string filename);

	std::string TextToBinaryString(std::string words);

	void printHelp();

	bool validateStorageSpace(char* imageFile, char* dataFile, unsigned int _width, unsigned int _height, bool compression);

	std::vector<unsigned int> generateNoise(CryptoPP::byte* seedPointer, size_t dataLength, size_t imageLength);

	void writeLengthHeader(long length, unsigned char* pixel);

	int readLengthHeader(unsigned char* pixel);

	void writeFilenameHeader(std::string fileName, unsigned char* pixel);

	std::string readFilenameHeader(unsigned char* pixel);

	void writeSeedHeader(CryptoPP::byte* seedPointer, unsigned char* pixel);

	CryptoPP::byte* readSeedHeader(unsigned char* pixel);

	void hideDataInImage(std::vector<unsigned char> data, CryptoPP::byte* seedPointer, unsigned char* pixel, std::vector<unsigned char> _image);

	std::vector<unsigned char> extractDataFromImage(int length, CryptoPP::byte* seedPointer, unsigned char* pixel, std::vector<unsigned char> _image);

	std::vector<unsigned char> Encrypt(CryptoPP::byte key[], CryptoPP::byte iv[], std::vector<unsigned char> data);

	std::vector<unsigned char> Decrypt(CryptoPP::byte key[], CryptoPP::byte iv[], std::vector<unsigned char> data);

	CryptoPP::byte* generateSHA256(std::string data);

	std::vector<CryptoPP::byte> zlibCompress(std::vector<CryptoPP::byte> input);

	std::vector<CryptoPP::byte> zlibDecompress(std::vector<CryptoPP::byte> input);

	std::vector<CryptoPP::byte> zopfliCompress(std::vector<CryptoPP::byte> input);

}
