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

#include "lodepng.h"

#include "SteganoPNG.h"
#include "ConfigurationManager.h"


#ifdef USEZOPFLI
#include "zopfli.h"
#endif // USEZOPFLI

int main(int argc, char* argv[]) {

	ConfigurationManager config = ConfigurationManager(argc, argv);

	ConfigurationManager::Mode mode = std::get<ConfigurationManager::Mode>(config.getOption(ConfigurationManager::Option::Mode));
	std::string imagePath = std::get<std::string>(config.getOption(ConfigurationManager::Option::ImagePath));
	std::string dataPath = std::get<std::string>(config.getOption(ConfigurationManager::Option::DataPath));
	bool encryption = std::get<bool>(config.getOption(ConfigurationManager::Option::Encryption));
	std::string password = std::get<std::string>(config.getOption(ConfigurationManager::Option::Password));
	bool disableCompression = std::get<bool>(config.getOption(ConfigurationManager::Option::DisableCompression));

	std::vector<unsigned char> _image;
	unsigned int _width;
	unsigned int _height;
	lodepng::State _state;

	if (mode != ConfigurationManager::Mode::HELP) {
		if (!(std::filesystem::exists(imagePath) && (std::filesystem::exists(dataPath) || dataPath.empty()))) {
			std::cout << "One or more specified files do not exist" << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	switch (mode) {
		case ConfigurationManager::Mode::HIDE:
		{

			SteganoPNG::decodeOneStep(imagePath.c_str(), &_image, &_width, &_height, &_state);

			SteganoPNG::validateStorageSpace(_image, (char*)dataPath.c_str(), _width, _height, _state, !disableCompression);

			std::vector<unsigned char> dataToHide = SteganoPNG::readAllBytes(dataPath);

			if (!disableCompression) {
#ifndef USEZOPFLI
				dataToHide = SteganoPNG::zlibCompress(dataToHide);
#else
				dataToHide = SteganoPNG::zopfliCompress(dataToHide);
#endif // USEZOPFLI
				dataToHide.shrink_to_fit();
			}

			unsigned char* pixel = _image.data();

			if (encryption) {
				CryptoPP::byte key[AES::MAX_KEYLENGTH];
				CryptoPP::byte iv[AES::BLOCKSIZE];

				memcpy(key, SteganoPNG::generateSHA256(password), sizeof(key));
				memcpy(iv, key, sizeof(iv));

				dataToHide = SteganoPNG::Encrypt(key, iv, dataToHide);
			}

			CryptoPP::byte* seed = new CryptoPP::byte[SHA256::DIGESTSIZE];
			memcpy(seed, SteganoPNG::generateSHA256(std::to_string(std::chrono::system_clock::now().time_since_epoch().count())), SHA256::DIGESTSIZE);

			SteganoPNG::writeLengthHeader((unsigned int)dataToHide.size(), pixel);
			SteganoPNG::writeFilenameHeader(SteganoPNG::getFileName(dataPath), pixel);
			SteganoPNG::writeSeedHeader(seed, pixel);
			SteganoPNG::hideDataInImage(dataToHide, seed, pixel, _image);

			SteganoPNG::encodeOneStep(imagePath.c_str(), _image, _width, _height, _state);

			break;
		}
		case ConfigurationManager::Mode::EXTRACT:
		{

			SteganoPNG::decodeOneStep(imagePath.c_str(), &_image, &_width, &_height, &_state);
			unsigned char* pixel = _image.data();

			int length = SteganoPNG::readLengthHeader(pixel);
			std::string filename = SteganoPNG::readFilenameHeader(pixel);
			CryptoPP::byte* seed = SteganoPNG::readSeedHeader(pixel);

			std::ofstream src(filename);
			if (!src) {
				std::cout << "This file does not contain a valid filename header. Are you sure it contains hidden data?" << std::endl;
				exit(EXIT_FAILURE);
			}

			std::vector<unsigned char> extractedData = SteganoPNG::extractDataFromImage(length, seed, pixel, _image);

			if (!disableCompression) {
				CryptoPP::byte key[AES::MAX_KEYLENGTH];
				CryptoPP::byte iv[AES::BLOCKSIZE];

				memcpy(key, SteganoPNG::generateSHA256(password), sizeof(key));
				memcpy(iv, key, sizeof(iv));
				try {
					//In case a password is specified with the -p argument but the data isnt SteganoPNG::Encrypted
					extractedData = SteganoPNG::Decrypt(key, iv, extractedData);
				}
				catch (Exception) {
					//ignore error silently
				}
			}

			try {
				//if the data was saved with the --no-compression flag this will cause an error and instead use the original value for extractedData
				extractedData = SteganoPNG::zlibDecompress(extractedData);
				extractedData.shrink_to_fit();
			}
			catch (Exception) {
				//ignore error silently
			}

			SteganoPNG::writeAllBytes(filename, extractedData);

			break;
		}
		case ConfigurationManager::Mode::TEST:
		{
			SteganoPNG::decodeOneStep(imagePath.c_str(), &_image, &_width, &_height, &_state);


			bool result = SteganoPNG::validateStorageSpace(_image, (char*)dataPath.c_str(), _width, _height, _state, !disableCompression);
			if (result) {
				std::cout << "The file " << SteganoPNG::getFileName(imagePath) << " contains enough pixels to hide all data of " << SteganoPNG::getFileName(dataPath) << " ." << std::endl;
				exit(EXIT_SUCCESS);
			}
			else {
				std::cout << "The file " << SteganoPNG::getFileName(imagePath) << " does not contain enough pixels to hide all data of " << SteganoPNG::getFileName(dataPath) << " ." << std::endl;
				exit(EXIT_FAILURE);
			}

			break;
		}
		case ConfigurationManager::Mode::HELP:
			[[fallthrough]];
		default:
		{
			SteganoPNG::printHelp();
			exit(EXIT_SUCCESS);
		}
	}
	return 0;
}

#pragma region Auxiliary

std::string SteganoPNG::getFileName(std::string filename) {
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

std::string SteganoPNG::TextToBinaryString(std::string words) {
	std::string binaryString = "";

	//Loop through chars in string, put them in a bitset and return the bitset for each char as a string.
	for (char& _char : words) {
		binaryString += std::bitset<8>(_char).to_string();
	}
	return binaryString;
}

void SteganoPNG::printHelp() {

	std::cout << "Syntax: SteganoPNG <command> <image.png> [data.xyz] [-p <password>] [--no-compression]" << std::endl << std::endl

		<< "Commands:" << std::endl
		<< "\ta" << "\t" << "Hide provided file in image" << std::endl
		<< "\tx" << "\t" << "Extract file hidden in image" << std::endl
		<< "\tt" << "\t" << "Verify if the image contains enough pixels for storage" << std::endl
		<< "\th" << "\t" << "Show this help screen" << std::endl << std::endl
		<< "Examples:" << std::endl
		<< "\tSteganoPNG a image.png FileYouWantToHide.xyz\t to hide \"FileYouWantToHide.xyz\" inside image.png" << std::endl
		<< "\tSteganoPNG x image.png\t\t\t\t to extract the file hidden in image.png" << std::endl
		<< "\tSteganoPNG t image.png FileYouWantToHide.xyz\t to verify that the image contains enough pixels for storage" << std::endl << std::endl
		<< "Use this software at your OWN risk" << std::endl
		;
}

bool SteganoPNG::validateStorageSpace(std::vector<CryptoPP::byte> _image, char* dataFile, unsigned int _width, unsigned int _height, lodepng::State _state, bool compression = true) {

	bool result = false;

	std::vector<unsigned char> data = SteganoPNG::readAllBytes(dataFile);

	if (compression) {
#ifndef USEZOPFLI
		data = SteganoPNG::zlibCompress(data);
#else
		data = SteganoPNG::zopfliCompress(data);
#endif // !USEZOPFLI

		
		data.shrink_to_fit();
	}

	size_t subpixelcount = _image.size();
	size_t dataLength = 8 * data.size();
	if ((subpixelcount - 2048 - 32) > dataLength) {
		result = true;
	}

	return result;
}

std::vector<unsigned int> SteganoPNG::generateNoise(CryptoPP::byte* seedPointer, size_t dataLength, size_t imageLength) {
	CryptoPP::byte seed[SHA256::DIGESTSIZE];
	memcpy(seed, seedPointer, sizeof(seed));

	std::seed_seq seed2(std::begin(seed), std::end(seed));
	std::mt19937 g(seed2);

	std::vector<unsigned int> noise(imageLength);
	std::iota(begin(noise), end(noise), 0);

	auto uniform = [](size_t a, std::mt19937& rng) {
		unsigned int rand = rng();
		unsigned int result = static_cast<unsigned int>((static_cast<float>(rand) / std::mt19937::max()) * a);
		return result;
	};

	unsigned int offset = 2336;

	noise.erase(noise.begin(), noise.begin() + offset);

	for (size_t i = noise.size() - 1; i > 0; --i) {
		unsigned int j = uniform(noise.size() - 1, g);
		std::swap(noise[i], noise[j]);
	}

	noise.resize((size_t)(dataLength * 8U));

	return noise;
}

#pragma endregion

#pragma region DiskIO

void SteganoPNG::decodeOneStep(const char* filename, std::vector<unsigned char> *_image, unsigned int *_width, unsigned int *_height, lodepng::State *_state) {
	std::vector<unsigned char> png;
	std::vector<unsigned char> image; //the raw pixels
	unsigned width, height;
	lodepng::State state; //optionally customize this one

	unsigned error = lodepng::load_file(png, filename); //load the image file with given filename

	lodepng_inspect(&width, &height, &state, &png[0], png.size()); //check if image is RGB instead of RGBA
	if (state.info_png.color.colortype == LodePNGColorType::LCT_RGB)
		state.info_raw.colortype = LodePNGColorType::LCT_RGB;

	if (!error) error = lodepng::decode(image, width, height, state, png);

	//if there's an error, display it
	if (error) {
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		exit(EXIT_FAILURE);
	}
		

	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
	//State state contains extra information about the PNG such as text chunks, ...
	*_image = image;
	*_width = width;
	*_height = height;
	*_state = state;
}

void SteganoPNG::encodeOneStep(const char* filename, const std::vector<unsigned char>& image, unsigned width, unsigned height, lodepng::State _state) {
	std::vector<unsigned char> png;

	_state.encoder.auto_convert = false;
	_state.encoder.filter_strategy = LodePNGFilterStrategy(_state.info_png.filter_method);
	_state.encoder.filter_palette_zero = false;
	
	_state.encoder.zlibsettings.btype = 2;
	_state.encoder.zlibsettings.use_lz77 = true;
	_state.encoder.zlibsettings.windowsize = 32768;
	_state.encoder.zlibsettings.nicematch = 258;
	_state.encoder.zlibsettings.lazymatching = 1;

	unsigned error = lodepng::encode(png, image, width, height, _state);
	if (!error) lodepng::save_file(png, filename);

	//if there's an error, display it
	if (error) std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
}

std::vector<unsigned char> SteganoPNG::readAllBytes(std::string fileName) {
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
		infile.read((char*)& buffer[0], length);
	}

	//Close instream
	infile.close();

	return buffer;
}

void SteganoPNG::writeAllBytes(std::string fileName, std::vector<unsigned char> data) {

	std::ofstream of;
	of.open(fileName, std::ios::out | std::ios::binary);
	of.write((char*)& data[0], data.size());
	of.close();

}

#pragma endregion

#pragma region Steganography

void SteganoPNG::hideDataInImage(std::vector<unsigned char> data, CryptoPP::byte* seedPointer, unsigned char* pixel, std::vector<unsigned char> _image) {

	std::vector<unsigned int> noise = SteganoPNG::generateNoise(seedPointer, data.size(), _image.size());

	for (int i = 0; i < (int)data.size(); i++) {
		//Create 8 bit buffer to store bits of each byte.
		std::bitset<8> buffer = data[i];
		//Counter to track which subpixel to read next.
		int progressOffset = i * 8;
		for (int64_t ii = 0; ii < 8; ii++) {
			//Create reversed index.
			int64_t reversedIndex = 7 - ii;
			pixel[noise.at(ii + progressOffset)] = SteganoPNG::setLastBit(pixel[noise.at(ii + progressOffset)], buffer[reversedIndex]);
		}
	}

}

std::vector<unsigned char> SteganoPNG::extractDataFromImage(int length, CryptoPP::byte* seedPointer, unsigned char* pixel, std::vector<unsigned char> _image) {

	//Initialize vector to hold extracted data.
	std::vector<unsigned char> data(length);

	//Create 8 bit bitset to store and convert the read bits.
	std::bitset<8> extractedByte;

	std::vector<unsigned int> noise = SteganoPNG::generateNoise(seedPointer, data.size(), _image.size());

	//Loop through as many bytes as length tells us to.
	for (int i = 0; i < length; i++) {
		//Counter to track which subpixel to read next.
		int progressOffset = i * 8;
		extractedByte.reset();
		//For each byte of extracted data loop through 8 subpixels (2 full pixels).
		for (int64_t ii = 7; ii >= 0; ii--) {
			//Set the bits of the header to the LSB of the read byte.
			//The bits are written to the bitset in reverse order as the bits in the bitset start with the LSB and the bits read from file start with MSB.
			extractedByte[ii] = SteganoPNG::getLastBit(pixel[noise.at((int64_t)7 - ii + progressOffset)]);
		}

		data[i] = (unsigned char)extractedByte.to_ulong();
	}

	return data;
}

#pragma endregion

#pragma region BitManipulation

unsigned char SteganoPNG::setLastBit(unsigned char byte, int bit) {

	//Create a mask of the original byte with 0xFE (0x11111110 or 254) via bitwise AND
	int maskedOriginal = (byte & 0xFE);

	//Create modified byte from the previously created mask and the bit that should be set via bitwise OR
	unsigned char modifiedByte = (maskedOriginal | bit);

	return modifiedByte;
}

int SteganoPNG::getLastBit(unsigned char byte) {

	//Read LSB (Least significant bit) via bitwise AND with 0x1 (0x00000001 or 1)
	int bit = (byte & 0x1);

	return bit;
}

#pragma endregion

#pragma region Metadata

void SteganoPNG::writeLengthHeader(long length, unsigned char* pixel) {

	//Create 32 bit bitset to contain header and initialize it with the value of length (how many bytes of data will be hidden in the image)
	std::bitset<32> header(length);

	for (int i = 0; i < 32; i++) {
		//create reversed index
		int reversedIndex = 31 - i;

		//Overwrite the byte at position i (0-31) of the decoded subpixel data with a modified byte. 
		//The LSB of the modified byte is set to the bit value of header[reversedIndex] as the header is written to file starting with the MSB and the bitset starts with the LSB.
		pixel[i] = SteganoPNG::setLastBit(pixel[i], header[reversedIndex]);
	}
}

int SteganoPNG::readLengthHeader(unsigned char* pixel) {

	//Create 32 bit bitset to contain the header thats read from file.
	std::bitset<32> headerBits;

	for (int i = 0; i < 32; i++) {
		//create reversed index
		int reversedIndex = 31 - i;

		//Set the bits of the header to the LSB of the read byte.
		//The bits are written to the bitset in reverse order as the bits in the bitset start with the LSB and the bits read from file start with MSB.
		headerBits[reversedIndex] = SteganoPNG::getLastBit(pixel[i]);
	}

	//Return the header bitset as int
	return headerBits.to_ulong();
}

void SteganoPNG::writeFilenameHeader(std::string fileName, unsigned char* pixel) {

	//Create 2048 bit bitset to contain filename header and initialize it with the value of filename.
	std::bitset<2048> header(SteganoPNG::TextToBinaryString(fileName));

	//Create offset to not overwrite length header.
	const int offset = 32;

	for (int i = 0; i < 2048; i++) {
		//create reversed index.
		int reversedIndex = 2047 - i;

		//Overwrite the byte at position i (32-2079) of the decoded subpixel data with a modified byte. 
		//The LSB of the modified byte is set to the bit value of header[reversedIndex] as the header is written to file starting with the MSB and the bitset starts with the LSB.
		pixel[i + offset] = SteganoPNG::setLastBit(pixel[i + offset], header[reversedIndex]);
	}
}

std::string SteganoPNG::readFilenameHeader(unsigned char* pixel) {

	//Create 2048 bit bitset to contain the header thats read from file.
	std::bitset<2048> headerBits;

	//Create offset to not read length header.
	const int offset = 32;

	for (int i = 0; i < 2048; i++) {
		//create reversed index
		int reversedIndex = 2047 - i;

		//Set the bits of the header to the LSB of the read byte.
		//The bits are written to the bitset in reverse order as the bits in the bitset start with the LSB and the bits read from file start with MSB.
		headerBits[reversedIndex] = SteganoPNG::getLastBit(pixel[i + offset]);
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
	output.erase(remove(output.begin(), output.end(), '\0'), output.end());

	//Return the header bitset as int
	return output;
}

void SteganoPNG::writeSeedHeader(CryptoPP::byte* seedPointer, unsigned char* pixel) {

	CryptoPP::byte* seed = new CryptoPP::byte[SHA256::DIGESTSIZE];
	memcpy(seed, seedPointer, (size_t)SHA256::DIGESTSIZE);

	//Create 256 bit bitset to contain header and initialize it with the value of length (how many bytes of data will be hidden in the image)
	std::bitset<8> seedHeader;

	//Create offset to not read filename header.
	const int offset = 2080;

	for (int i = 0; i < 32; i++) {
		seedHeader = std::bitset<8>(seed[i]);
		//Counter to track which subpixel to read next.
		int progressOffset = i * 8;
		for (int ii = 0; ii < 8; ii++) {
			//create reversed index
			int reversedIndex = 7 - ii;

			//Overwrite the byte at position i (2080-2336) of the decoded subpixel data with a modified byte. 
			//The LSB of the modified byte is set to the bit value of seedHeader[reversedIndex] as the header is written to file starting with the MSB and the bitset starts with the LSB.
			pixel[ii + offset + progressOffset] = SteganoPNG::setLastBit(pixel[ii + offset + progressOffset], seedHeader[reversedIndex]);
		}

	}

	
}

CryptoPP::byte* SteganoPNG::readSeedHeader(unsigned char* pixel) {

	//Create 8 bit bitset to create a byte.
	std::bitset<8> headerByte;

	//create byte array to store read seed header
	CryptoPP::byte* seed = new CryptoPP::byte[SHA256::DIGESTSIZE];

	//Create offset to not read filename header.
	const int offset = 2080;

	for (int i = 0; i < 32; i++) {
		//Counter to track which subpixel to read next.
		int progressOffset = i * 8;
		for (int ii = 0; ii < 8; ii++) {
			//create reversed index
			int reversedIndex = 7 - ii;

			//Set the bits of the header to the LSB of the read byte.
			//The bits are written to the bitset in reverse order as the bits in the bitset start with the LSB and the bits read from file start with MSB.
			headerByte[reversedIndex] = SteganoPNG::getLastBit(pixel[ii + offset + progressOffset]);
		}

		seed[i] = (unsigned char)headerByte.to_ulong();
	}

	//Return the header bitset as int
	return seed;
}

#pragma endregion

#pragma region Crypto

std::vector<unsigned char> SteganoPNG::Encrypt(CryptoPP::byte key[], CryptoPP::byte iv[], std::vector<unsigned char> data) {

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

	std::vector<unsigned char> cipher;

	// Make room for padding
	cipher.resize(data.size() + AES::BLOCKSIZE);
	ArraySink cs(&cipher[0], cipher.size());

	(void)ArraySource(data.data(), data.size(), true,
		new StreamTransformationFilter(enc, new Redirector(cs), StreamTransformationFilter::PKCS_PADDING));

	// Set cipher text length now that its known
	cipher.resize(cs.TotalPutLength());

	return cipher;
}

std::vector<unsigned char> SteganoPNG::Decrypt(CryptoPP::byte key[], CryptoPP::byte iv[], std::vector<unsigned char> data) {

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

	std::vector<unsigned char> recover;

	// Recovered text will be less than cipher text
	recover.resize(data.size());
	ArraySink rs(&recover[0], recover.size());

	(void)ArraySource(data.data(), data.size(), true,
		new StreamTransformationFilter(dec, new Redirector(rs), StreamTransformationFilter::PKCS_PADDING));

	// Set recovered text length now that its known
	recover.resize(rs.TotalPutLength());

	return recover;
}

CryptoPP::byte* SteganoPNG::generateSHA256(std::string data)
{
	CryptoPP::byte const* pbData = (CryptoPP::byte*)data.data();
	size_t nDataLen = data.size();
	CryptoPP::byte* abDigest = new CryptoPP::byte[SHA256::DIGESTSIZE];

	SHA256().CalculateDigest(abDigest, pbData, nDataLen);

	return abDigest;
}

#pragma endregion

#pragma region Compression

std::vector<CryptoPP::byte> SteganoPNG::zlibDecompress(std::vector<CryptoPP::byte> input) {
	ZlibDecompressor zipper;
	zipper.Put((CryptoPP::byte*)input.data(), input.size());
	zipper.MessageEnd();

	word64 avail = zipper.MaxRetrievable();
	std::vector<CryptoPP::byte> decompressed;
	decompressed.resize(avail);

	zipper.Get(&decompressed[0], decompressed.size());

	return decompressed;
}

#ifdef USEZOPFLI
std::vector<CryptoPP::byte> SteganoPNG::zopfliCompress(std::vector<CryptoPP::byte> input) {
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

	std::vector<unsigned char> output(temp, temp + size);
	output.shrink_to_fit();

	return output;

}
#else
std::vector<CryptoPP::byte> SteganoPNG::zlibCompress(std::vector<CryptoPP::byte> input) {
	ZlibCompressor zipper;
	zipper.Put((CryptoPP::byte*)input.data(), input.size());
	zipper.MessageEnd();

	word64 avail = zipper.MaxRetrievable();
	if (avail < 9)
		throw std::runtime_error("ZLib was not supplied with enough data");
	std::vector<CryptoPP::byte> compressed;
	compressed.resize(avail);

	zipper.Get(&compressed[0], compressed.size());
	return compressed;

}
#endif // !USEZOPFLI

#pragma endregion

