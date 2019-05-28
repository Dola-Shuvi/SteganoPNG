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

std::string getFileName(std::string filename);

void writeLengthHeader(long length, unsigned char *pixel);

int readLengthHeader(unsigned char *pixel);

void writeWidthHeader(long length, unsigned char* pixel);

int readWidthHeader(unsigned char* pixel);

void writeHeightHeader(long length, unsigned char* pixel);

int readHeightHeader(unsigned char* pixel);

