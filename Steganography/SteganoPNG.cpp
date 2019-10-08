#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <bitset>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <random>
#include <numeric>
#include <chrono>
using namespace std;

//#define USEZOPFLI

#include "cryptlib.h"
#include "filters.h"
#include "files.h"
#include "modes.h"
#include "queue.h"
#include "aes.h"
#include "sha.h"
#include "zlib.h"
using namespace CryptoPP;

#include "SteganoPNG.h"
#include "lodepng.h"

#ifdef USEZOPFLI
#include "zopfli.h"
#endif // USEZOPFLI


vector<unsigned char> _image;
unsigned int _width;
unsigned int _height;

int main(int argc, char** argv) {
	if (((argc != 3 && argc != 4 && argc != 5 && argc != 6 && argc != 7) || (strcmp(argv[1], "h") == 0) || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
		printHelp();
		exit(EXIT_SUCCESS);
	}

	if (strcmp(argv[1], "a") == 0) {
		if (!(filesystem::exists(argv[2]) && filesystem::exists(argv[3]))) {
			cout << "One or more specified files do not exist" << endl;
			exit(EXIT_FAILURE);
		}

		decodeOneStep(argv[2]);

		bool compression = true;
		if (argc > 4) {
			if (strcmp(argv[4], "--no-compression") == 0) compression = false;
		}
		else if (argc > 6) {
			if (strcmp(argv[6], "--no-compression") == 0) compression = false;
		}

		validateStorageSpace(argv[2], argv[3], compression);

		vector<unsigned char> dataToHide = readAllBytes(argv[3]);

		if (compression) {

#ifndef USEZOPFLI
			dataToHide = zlibCompress(dataToHide);
#else
			dataToHide = zopfliCompress(dataToHide);
#endif // USEZOPFLI
			dataToHide.shrink_to_fit();
		}

		unsigned char* pixel = _image.data();

		if (argc == 6 || argc == 7) {
			if (strcmp(argv[4], "-p") == 0) {
				CryptoPP::byte key[AES::MAX_KEYLENGTH];
				CryptoPP::byte iv[AES::BLOCKSIZE];

				memcpy(key, generateSHA256(argv[5]), sizeof(key));
				memcpy(iv, key, sizeof(iv));

				dataToHide = Encrypt(key, iv, dataToHide);
			}
		}

		CryptoPP::byte* seed = new CryptoPP::byte[SHA256::DIGESTSIZE];
		memcpy(seed, generateSHA256(to_string(chrono::system_clock::now().time_since_epoch().count())), sizeof(seed));

		writeLengthHeader((unsigned int)dataToHide.size(), pixel);
		writeFilenameHeader(getFileName(string(argv[3])), pixel);
		writeSeedHeader(seed, pixel);
		hideDataInImage(dataToHide, seed, pixel);

		encodeOneStep(argv[2], _image, _width, _height);

		exit(EXIT_SUCCESS);
	}
	else if (strcmp(argv[1], "x") == 0){

		if (!filesystem::exists(argv[2])) {
			cout << "One or more specified files do not exist" << endl;
			exit(EXIT_FAILURE);
		}

		bool compression = true;
		if (argc > 3) {
			if (strcmp(argv[3], "--no-compression") == 0) compression = false;
		}
		else if (argc > 5) {
			if (strcmp(argv[5], "--no-compression") == 0) compression = false;
		}

		decodeOneStep(argv[2]);
		unsigned char* pixel = _image.data();

		int length = readLengthHeader(pixel);
		string filename = readFilenameHeader(pixel);
		CryptoPP::byte* seed = readSeedHeader(pixel);

		ofstream src(filename);
		if (!src) {
			cout << "This file does not contain a valid filename header. Are you sure it contains hidden data?" << endl;
			exit(EXIT_FAILURE);
		}

		vector<unsigned char> extractedData = extractDataFromImage(length, seed, pixel);

		if (argc == 5 || argc == 6) {
			if (strcmp(argv[3], "-p") == 0) {
				CryptoPP::byte key[AES::MAX_KEYLENGTH];
				CryptoPP::byte iv[AES::BLOCKSIZE];

				memcpy(key, generateSHA256(argv[4]), sizeof(key));
				memcpy(iv, key, sizeof(iv));
				try {
					//In case a password is specified with the -p argument but the data isnt encrypted
					extractedData = Decrypt(key, iv, extractedData);
				}
				catch (Exception ex) {
					//ignore error silently
				}
				
			}
		}

			try {
				//if the data was saved with the --no-compression flag this will cause an error and instead use the original value for extractedData
				extractedData = zlibDecompress(extractedData);
				extractedData.shrink_to_fit();
			}
			catch (Exception ex) {
				//ignore error silently
			}

		writeAllBytes(filename, extractedData);

	}
	else if (strcmp(argv[1], "t") == 0) {

		if (!(filesystem::exists(argv[2]) && filesystem::exists(argv[3]))){
			cout << "One or more specified files do not exist" << endl;
			exit(EXIT_FAILURE);
		}

		decodeOneStep(argv[2]);

		bool compression = true;
		if (argc > 4) {
			if (strcmp(argv[4], "--no-compression")) compression = false;
		}
		

		validateStorageSpace(argv[2], argv[3], compression);

		cout << "The file " << getFileName(argv[2]) << " contains enough pixels to hide all data of " << getFileName(argv[2]) << " ." << endl;
		exit(EXIT_SUCCESS);
	}
	else {
		printHelp();
		exit(EXIT_FAILURE);
	}

	return 0;
}

#pragma region Auxilary

string getFileName(string filename) {
	//Get index of last slash in path.
	const size_t last_slash_idx = filename.find_last_of("\\/");

	//Check if slash is the last character in the path.
	if (string::npos != last_slash_idx)
	{
		//Erase everything before the last slash, including last slash.
		filename.erase(0, last_slash_idx + 1);
	}
	return filename;
}

string TextToBinaryString(string words) {
	string binaryString = "";

	//Loop through chars in string, put them in a bitset and return the bitset for each char as a string.
	for (char& _char : words) {
		binaryString += bitset<8>(_char).to_string();
	}
	return binaryString;
}

void printHelp() {

	cout << "Syntax: SteganoPNG <command> <image.png> [data.xyz] [-p <password>] [--no-compression]" << endl << endl

		<< "Commands:" << endl
		<< "\ta" << "\t" << "Hide provided file in image" << endl
		<< "\tx" << "\t" << "Extract file hidden in image" << endl
		<< "\tt" << "\t" << "Verify if the image contains enough pixels for storage" << endl
		<< "\th" << "\t" << "Show this help screen" << endl << endl
		<< "Examples:" << endl
		<< "\tSteganoPNG a image.png FileYouWantToHide.xyz\t to hide \"FileYouWantToHide.xyz\" inside image.png" << endl
		<< "\tSteganoPNG x image.png\t\t\t\t to extract the file hidden in image.png" << endl
		<< "\tSteganoPNG t image.png FileYouWantToHide.xyz\t to verify that the image contains enough pixels for storage" << endl << endl
		<< "Use this software at your OWN risk" << endl
		;
}

bool validateStorageSpace(char* imageFile, char* dataFile, bool compression = true) {

	bool result = false;

	vector<unsigned char> data = readAllBytes(dataFile);

	if (compression) {
		data = zlibCompress(data);
		data.shrink_to_fit();
	}

	size_t subpixelcount = (size_t)_width * (size_t)_height * 4;
	size_t dataLength = 8 * data.size();
	if ((subpixelcount - 2048 - 32) > dataLength) {
		result = true;
	}
	else {
		cout << "The file " << getFileName(imageFile) << " does not contain enough pixels to hide all data of " << getFileName(dataFile) << " ." << endl;
		exit(EXIT_FAILURE);
	}

	return result;
}

vector<unsigned int> generateNoise(CryptoPP::byte* seedPointer, size_t dataLength, size_t imageLength) {
	CryptoPP::byte seed[SHA256::DIGESTSIZE];
	memcpy(seed, seedPointer, sizeof(seed));

	seed_seq seed2(begin(seed), end(seed));
	mt19937 g(seed2);

	vector<unsigned int> noise(imageLength);
	iota(begin(noise), end(noise), 0);

	unsigned int offset = 2336;
	noise.erase(noise.begin(), noise.begin() + offset);

	shuffle(begin(noise), end(noise), g);

	noise.resize((size_t)(dataLength * 8U));

	return noise;
}

#pragma endregion

#pragma region DiskIO

void decodeOneStep(const char* filename) {
	vector<unsigned char> image; //the raw pixels
	unsigned width, height;

	//decode
	unsigned error = lodepng::decode(image, width, height, filename);

	//if there's an error, display it
	if (error) {
		cout << "decoder error " << error << ": " << lodepng_error_text(error) << endl;
		cout << "The image you provided is corrupted or not supported. Please provide a PNG file." << endl;
		exit(EXIT_FAILURE);
	}

	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
	_image = image;
	_width = width;
	_height = height;
}

void encodeOneStep(const char* filename, vector<unsigned char>& image, unsigned width, unsigned height) {
	//Encode the image
	unsigned error = lodepng::encode(filename, image, width, height);

	//if there's an error, display it
	if (error) cout << "encoder error " << error << ": " << lodepng_error_text(error) << endl;
}

vector<unsigned char> readAllBytes(string fileName) {
	//Open file
	ifstream infile(fileName, ios::in | ios::binary);
	vector<unsigned char> buffer;

	//Get length of file
	infile.seekg(0, infile.end);
	size_t length = infile.tellg();
	infile.seekg(0, infile.beg);

	//Read file
	if (length > 0) {
		buffer.resize(length);
		infile.read((char*)& buffer[0], length);
	}

	//Close instream
	infile.close();

	return buffer;
}

void writeAllBytes(string fileName, vector<unsigned char> data) {

	ofstream of;
	of.open(fileName, ios::out | ios::binary);
	of.write((char*)& data[0], data.size());
	of.close();

}

#pragma endregion

#pragma region Steganography

void hideDataInImage(vector<unsigned char> data, CryptoPP::byte* seedPointer, unsigned char* pixel) {

	//Create reversed index.
	int64_t reversedIndex;

	//Create 8 bit buffer to store bits of each byte.
	bitset<8> buffer;

	//Counter to track which subpixel to read next.
	int progressOffset;

	vector<unsigned int> noise = generateNoise(seedPointer, data.size(), _image.size());

	for (int i = 0; i < (int)data.size(); i++) {
		buffer = data[i];
		progressOffset = i * 8;
		for (int64_t ii = 0; ii < 8; ii++) {
			reversedIndex = 7 - ii;
			pixel[noise.at(ii + progressOffset)] = setLastBit(pixel[noise.at(ii + progressOffset)], buffer[reversedIndex]);
		}
	}

}

vector<unsigned char> extractDataFromImage(int length, CryptoPP::byte* seedPointer, unsigned char* pixel) {

	//Initialize vector to hold extracted data.
	vector<unsigned char> data(length);

	//Create 8 bit bitset to store and convert the read bits.
	bitset<8> extractedByte;

	//Counter to track which subpixel to read next.
	int progressOffset;

	vector<unsigned int> noise = generateNoise(seedPointer, data.size(), _image.size());

	//Loop through as many bytes as length tells us to.
	for (int i = 0; i < length; i++) {
		progressOffset = i * 8;
		extractedByte.reset();
		//For each byte of extracted data loop through 8 subpixels (2 full pixels).
		for (int64_t ii = 7; ii >= 0; ii--) {
			//Set the bits of the header to the LSB of the read byte.
			//The bits are written to the bitset in reverse order as the bits in the bitset start with the LSB and the bits read from file start with MSB.
			extractedByte[ii] = getLastBit(pixel[noise.at((int64_t)7 - ii + progressOffset)]);
		}

		data[i] = (unsigned char)extractedByte.to_ulong();
	}

	return data;
}

#pragma endregion

#pragma region BitManipulation

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

#pragma endregion

#pragma region Metadata

void writeLengthHeader(long length, unsigned char* pixel) {

	//Create 32 bit bitset to contain header and initialize it with the value of length (how many bytes of data will be hidden in the image)
	bitset<32> header(length);

	//create reversed index
	int reversedIndex;

	for (int i = 0; i < 32; i++) {
		reversedIndex = 31 - i;

		//Overwrite the byte at position i (0-31) of the decoded subpixel data with a modified byte. 
		//The LSB of the modified byte is set to the bit value of header[reversedIndex] as the header is written to file starting with the MSB and the bitset starts with the LSB.
		pixel[i] = setLastBit(pixel[i], header[reversedIndex]);
	}
}

int readLengthHeader(unsigned char* pixel) {

	//Create 32 bit bitset to contain the header thats read from file.
	bitset<32> headerBits;

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

void writeFilenameHeader(string fileName, unsigned char* pixel) {

	//Create 2048 bit bitset to contain filename header and initialize it with the value of filename.
	bitset<2048> header(TextToBinaryString(fileName));

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

string readFilenameHeader(unsigned char* pixel) {

	//Create 2048 bit bitset to contain the header thats read from file.
	bitset<2048> headerBits;

	//Create offset to not read length header.
	const int offset = 32;

	//create reversed index
	int reversedIndex;

	for (int i = 0; i < 2048; i++) {
		reversedIndex = 2047 - i;

		//Set the bits of the header to the LSB of the read byte.
		//The bits are written to the bitset in reverse order as the bits in the bitset start with the LSB and the bits read from file start with MSB.
		headerBits[reversedIndex] = getLastBit(pixel[i + offset]);
	}

	//Convert binary to ascii
	stringstream sstream(headerBits.to_string());
	string output;
	while (sstream.good())
	{
		bitset<8> bits;
		sstream >> bits;
		char c = char(bits.to_ulong());
		output += c;
	}
	//Remove all NUL chars from output
	output.erase(remove(output.begin(), output.end(), '\0'), output.end());

	//Return the header bitset as int
	return output;
}

void writeSeedHeader(CryptoPP::byte* seedPointer, unsigned char* pixel) {

	CryptoPP::byte* seed = new CryptoPP::byte[SHA256::DIGESTSIZE];
	memcpy(seed, seedPointer, (size_t)SHA256::DIGESTSIZE);

	//Create 256 bit bitset to contain header and initialize it with the value of length (how many bytes of data will be hidden in the image)
	bitset<8> seedHeader;

	//create reversed index
	int reversedIndex;

	//Create offset to not read filename header.
	const int offset = 2080;

	//Counter to track which subpixel to read next.
	int progressOffset;

	for (int i = 0; i < 32; i++) {
		seedHeader = bitset<8>(seed[i]);
		progressOffset = i * 8;
		for (int ii = 0; ii < 8; ii++) {
			reversedIndex = 7 - ii;

			//Overwrite the byte at position i (2080-2336) of the decoded subpixel data with a modified byte. 
			//The LSB of the modified byte is set to the bit value of seedHeader[reversedIndex] as the header is written to file starting with the MSB and the bitset starts with the LSB.
			pixel[ii + offset + progressOffset] = setLastBit(pixel[ii + offset + progressOffset], seedHeader[reversedIndex]);
		}

	}

	
}

CryptoPP::byte* readSeedHeader(unsigned char* pixel) {

	//Create 8 bit bitset to create a byte.
	bitset<8> headerByte;

	//create reversed index
	int reversedIndex;

	//create byte array to store read seed header
	CryptoPP::byte* seed = new CryptoPP::byte[SHA256::DIGESTSIZE];

	//Create offset to not read filename header.
	const int offset = 2080;

	//Counter to track which subpixel to read next.
	int progressOffset;

	for (int i = 0; i < 32; i++) {
		progressOffset = i * 8;
		for (int ii = 0; ii < 8; ii++) {
			reversedIndex = 7 - ii;

			//Set the bits of the header to the LSB of the read byte.
			//The bits are written to the bitset in reverse order as the bits in the bitset start with the LSB and the bits read from file start with MSB.
			headerByte[reversedIndex] = getLastBit(pixel[ii + offset + progressOffset]);
		}

		seed[i] = (unsigned char)headerByte.to_ulong();
	}

	//Return the header bitset as int
	return seed;
}

#pragma endregion

#pragma region Crypto

vector<unsigned char> Encrypt(CryptoPP::byte key[], CryptoPP::byte iv[], vector<unsigned char> data) {

	CryptoPP::byte keycopy[AES::MAX_KEYLENGTH];
	memcpy(keycopy, key, sizeof(keycopy));

	CryptoPP::byte ivcopy[AES::BLOCKSIZE];
	memcpy(ivcopy, iv, sizeof(ivcopy));

	CBC_Mode<AES>::Encryption enc;
	enc.SetKeyWithIV(keycopy, sizeof(keycopy), ivcopy, sizeof(ivcopy));
	memset(key, 0x00, sizeof(keycopy));
	memset(iv, 0x00, sizeof(ivcopy));
	memset(keycopy, 0x00, sizeof(keycopy));
	memset(ivcopy, 0x00, sizeof(ivcopy));

	vector<unsigned char> cipher;

	// Make room for padding
	cipher.resize(data.size() + AES::BLOCKSIZE);
	ArraySink cs(&cipher[0], cipher.size());

	(void)ArraySource(data.data(), data.size(), true,
		new StreamTransformationFilter(enc, new Redirector(cs), StreamTransformationFilter::PKCS_PADDING));

	// Set cipher text length now that its known
	cipher.resize(cs.TotalPutLength());

	return cipher;
}

vector<unsigned char> Decrypt(CryptoPP::byte key[], CryptoPP::byte iv[], vector<unsigned char> data) {

	CryptoPP::byte keycopy[AES::MAX_KEYLENGTH];
	memcpy(keycopy, key, sizeof(keycopy));

	CryptoPP::byte ivcopy[AES::BLOCKSIZE];
	memcpy(ivcopy, iv, sizeof(ivcopy));

	CBC_Mode<AES>::Decryption dec;
	dec.SetKeyWithIV(keycopy, sizeof(keycopy), ivcopy, sizeof(ivcopy));
	memset(key, 0x00, sizeof(keycopy));
	memset(iv, 0x00, sizeof(ivcopy));
	memset(keycopy, 0x00, sizeof(keycopy));
	memset(ivcopy, 0x00, sizeof(ivcopy));

	vector<unsigned char> recover;

	// Recovered text will be less than cipher text
	recover.resize(data.size());
	ArraySink rs(&recover[0], recover.size());

	(void)ArraySource(data.data(), data.size(), true,
		new StreamTransformationFilter(dec, new Redirector(rs), StreamTransformationFilter::PKCS_PADDING));

	// Set recovered text length now that its known
	recover.resize(rs.TotalPutLength());

	return recover;
}

CryptoPP::byte* generateSHA256(string data)
{
	CryptoPP::byte const* pbData = (CryptoPP::byte*)data.data();
	size_t nDataLen = data.size();
	CryptoPP::byte* abDigest = new CryptoPP::byte[SHA256::DIGESTSIZE];

	SHA256().CalculateDigest(abDigest, pbData, nDataLen);

	return abDigest;
}

#pragma endregion

#pragma region Compression

vector<CryptoPP::byte> zlibCompress(vector<CryptoPP::byte> input) {
	ZlibCompressor zipper;
	zipper.Put((CryptoPP::byte*)input.data(), input.size());
	zipper.MessageEnd();

	word64 avail = zipper.MaxRetrievable();
	if (avail)
	{
		vector<CryptoPP::byte> compressed;
		compressed.resize(avail);

		zipper.Get(&compressed[0], compressed.size());
		return compressed;
	}
	else {
		cout << "A fatal error has occured during compression." << endl;
		exit(EXIT_FAILURE);
	}
}

vector<CryptoPP::byte> zlibDecompress(vector<CryptoPP::byte> input) {
	ZlibDecompressor zipper;
	zipper.Put((CryptoPP::byte*)input.data(), input.size());
	zipper.MessageEnd();

	word64 avail = zipper.MaxRetrievable();
	vector<CryptoPP::byte> decompressed;
	decompressed.resize(avail);

	zipper.Get(&decompressed[0], decompressed.size());

	return decompressed;
}

#ifdef USEZOPFLI
vector<CryptoPP::byte> zopfliCompress(vector<CryptoPP::byte> input) {
	ZopfliOptions options;
	ZopfliInitOptions(&options);

	if (input.size() > 10000000) {
		options.blocksplittingmax = 32;
		options.numiterations = 8;
	}
	else if (input.size() > 1000000) {
		options.blocksplittingmax = 32;
		options.numiterations = 16;
	}
	else {
		options.blocksplittingmax = 32;
		options.numiterations = 32;
	}



	size_t size = 0;
	unsigned char* temp;

	ZopfliCompress(&options, ZOPFLI_FORMAT_ZLIB, input.data(), input.size(), &temp, &size);

	vector<unsigned char> output(temp, temp + size);
	output.shrink_to_fit();

	return output;

}
#endif // !USEZOPFLI

#pragma endregion

