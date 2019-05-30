#include <stdlib.h>
#include <stdio.h>
#include "lodepng.h"
#include <iostream>
#include <bitset>
#include <fstream>

void decodeOneStep(const char* filename);

void encodeOneStep(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height);

unsigned char setLastBit(unsigned char byte, int bit);

int getLastBit(unsigned char byte);

std::vector<unsigned char> readAllBytes(std::string fileName);

void writeAllBytes(std::string fileName, std::vector<unsigned char> data);

std::string getFileName(std::string filename);

std::string TextToBinaryString(std::string words);

void printHelp();

bool validateStorageSpace(char* imageFile, char* dataFile);

void writeLengthHeader(long length, unsigned char *pixel);

int readLengthHeader(unsigned char *pixel);

void writeFilenameHeader(std::string fileName, unsigned char* pixel);

std::string readFilenameHeader(unsigned char* pixel);

void hideDataInImage(std::vector<unsigned char> data, unsigned char* pixel);

std::vector<unsigned char> extractDataFromImage(int length, unsigned char* pixel);

