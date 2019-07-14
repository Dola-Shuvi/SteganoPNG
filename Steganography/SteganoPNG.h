#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <bitset>
#include <fstream>
using namespace std;

#define NOCRYPTOPP

#ifndef NOCRYPTOPP
#include "config.h"
#endif // !NOCRYPTOPP	

#include "lodepng.h"

void decodeOneStep(const char* filename);

void encodeOneStep(const char* filename, vector<unsigned char>& image, unsigned width, unsigned height);

unsigned char setLastBit(unsigned char byte, int bit);

int getLastBit(unsigned char byte);

vector<unsigned char> readAllBytes(string fileName);

void writeAllBytes(string fileName, vector<unsigned char> data);

string getFileName(string filename);

string TextToBinaryString(string words);

void printHelp();

bool validateStorageSpace(char* imageFile, char* dataFile);

void writeLengthHeader(long length, unsigned char *pixel);

int readLengthHeader(unsigned char *pixel);

void writeFilenameHeader(string fileName, unsigned char* pixel);

string readFilenameHeader(unsigned char* pixel);

void hideDataInImage(vector<unsigned char> data, unsigned char* pixel);

vector<unsigned char> extractDataFromImage(int length, unsigned char* pixel);

#ifndef NOCRYPTOPP
vector<unsigned char> Encrypt(CryptoPP::byte key[], CryptoPP::byte iv[], vector<unsigned char> data);

vector<unsigned char> Decrypt(CryptoPP::byte key[], CryptoPP::byte iv[], vector<unsigned char> data);

CryptoPP::byte* generateSHA256(string data);

#endif // !NOCRYPTOPP