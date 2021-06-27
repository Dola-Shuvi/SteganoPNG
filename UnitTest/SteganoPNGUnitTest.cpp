#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <iostream>
#include <string>
#include <iomanip>
#include <random>
#include <numeric>
using namespace std;

#include <CryptoPP/cryptlib.h>
#include <CryptoPP/filters.h>
#include <CryptoPP/files.h>
#include <CryptoPP/modes.h>
#include <CryptoPP/queue.h>
#include <CryptoPP/aes.h>
#include <CryptoPP/sha.h>
#include <CryptoPP/zlib.h>
#include <CryptoPP/hex.h>
using namespace CryptoPP;

#include "..\SteganoPNG.h"
#include "..\ConfigurationManager.h"

#define TEST_CASE_DIRECTORY GetDirectoryName(__FILE__)

string GetDirectoryName(string path) {
	const size_t last_slash_idx = path.rfind('\\');
	if (std::string::npos != last_slash_idx)
	{
		return path.substr(0, last_slash_idx + 1);
	}
	return "";
}

void HideOnly(ConfigurationManager config) {

	std::string imagePath = std::get<std::string>(config.getOption(ConfigurationManager::Option::ImagePath));
	std::string dataPath = std::get<std::string>(config.getOption(ConfigurationManager::Option::DataPath));
	bool encryption = std::get<bool>(config.getOption(ConfigurationManager::Option::Encryption));
	std::string password = std::get<std::string>(config.getOption(ConfigurationManager::Option::Password));
	bool disableCompression = std::get<bool>(config.getOption(ConfigurationManager::Option::DisableCompression));

	std::vector<unsigned char> _image;
	unsigned int _width;
	unsigned int _height;
	lodepng::State _state;

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

	CryptoPP::byte* seed = new CryptoPP::byte[SHA256::DIGESTSIZE];
	memcpy(seed, SteganoPNG::generateSHA256(std::to_string(std::chrono::system_clock::now().time_since_epoch().count())), SHA256::DIGESTSIZE);

	if (encryption) {
		CryptoPP::byte key[AES::MAX_KEYLENGTH];
		CryptoPP::byte iv[AES::BLOCKSIZE];

		memcpy(key, SteganoPNG::generateSHA256(password), sizeof(key));
		memcpy(iv, key, sizeof(iv));

		dataToHide = SteganoPNG::Encrypt(key, iv, dataToHide);
	}

	SteganoPNG::writeLengthHeader((unsigned int)dataToHide.size(), pixel);
	SteganoPNG::writeFilenameHeader(SteganoPNG::getFileName(dataPath), pixel);
	SteganoPNG::writeSeedHeader(seed, pixel);
	SteganoPNG::hideDataInImage(dataToHide, seed, pixel, _image);

	SteganoPNG::encodeOneStep(imagePath.c_str(), _image, _width, _height, _state);

	remove(dataPath.c_str());

}

std::vector<unsigned char> ExtractOnly(ConfigurationManager config) {

	std::string imagePath = std::get<std::string>(config.getOption(ConfigurationManager::Option::ImagePath));
	std::string dataPath = std::get<std::string>(config.getOption(ConfigurationManager::Option::DataPath));
	bool encryption = std::get<bool>(config.getOption(ConfigurationManager::Option::Encryption));
	std::string password = std::get<std::string>(config.getOption(ConfigurationManager::Option::Password));
	bool disableCompression = std::get<bool>(config.getOption(ConfigurationManager::Option::DisableCompression));

	std::vector<unsigned char> _image;
	unsigned int _width;
	unsigned int _height;
	lodepng::State _state;

	SteganoPNG::decodeOneStep(imagePath.c_str(), &_image, &_width, &_height, &_state);
	unsigned char* pixel = _image.data();

	int length = SteganoPNG::readLengthHeader(pixel);
	std::string filename = SteganoPNG::readFilenameHeader(pixel);
	CryptoPP::byte* seed = SteganoPNG::readSeedHeader(pixel);

	std::ofstream src(filename);

	std::vector<unsigned char> extractedData = SteganoPNG::extractDataFromImage(length, seed, pixel, _image);

	if (encryption) {
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
	return extractedData;

}

std::vector<unsigned char> HideAndExtract(const ConfigurationManager &config) {

	HideOnly(config);

	std::vector<unsigned char> extractedData = ExtractOnly(config);

	return extractedData;
}

std::vector<unsigned char> InitializeTestFiles(std::string testfile, std::string imagefile) {

	ifstream source(std::string(TEST_CASE_DIRECTORY) + "circle.png", ios::binary);
	ofstream dest(imagefile, ios::binary);
	dest << source.rdbuf();
	source.close();
	dest.close();

	std::string plain_string;
	for (int i = 0; i < 63; i++) {
		plain_string += "ConfigurationTest" + std::to_string(i) + "\n";
	}
	std::vector<unsigned char> plain_vector(plain_string.begin(), plain_string.end());
	SteganoPNG::writeAllBytes(testfile, plain_vector);

	return plain_vector;
}

bool ValidateStorageSpace(ConfigurationManager config) {

	std::string imagePath = std::get<std::string>(config.getOption(ConfigurationManager::Option::ImagePath));
	std::string dataPath = std::get<std::string>(config.getOption(ConfigurationManager::Option::DataPath));
	bool disableCompression = std::get<bool>(config.getOption(ConfigurationManager::Option::DisableCompression));

	std::vector<unsigned char> _image;
	unsigned int _width;
	unsigned int _height;
	lodepng::State _state;

	SteganoPNG::decodeOneStep(imagePath.c_str(), &_image, &_width, &_height, &_state);

	return SteganoPNG::validateStorageSpace(_image, (char*)dataPath.c_str(), _width, _height, _state, !disableCompression);
}

void RunAsserts(ConfigurationManager config, ConfigurationManager::Mode _mode, string _imagefile, string _testfile, bool _Encryption, string _password, bool _disableCompression) {
	Assert::AreEqual((int)_mode, (int)std::get<ConfigurationManager::Mode>(config.getOption(ConfigurationManager::Option::Mode)));
	Assert::AreEqual(_imagefile, std::get<std::string>(config.getOption(ConfigurationManager::Option::ImagePath)));
	Assert::AreEqual(_testfile, std::get<std::string>(config.getOption(ConfigurationManager::Option::DataPath)));
	Assert::AreEqual(_Encryption, std::get<bool>(config.getOption(ConfigurationManager::Option::Encryption)));
	Assert::AreEqual(_password, std::get<std::string>(config.getOption(ConfigurationManager::Option::Password)));
	Assert::AreEqual(_disableCompression, std::get<bool>(config.getOption(ConfigurationManager::Option::DisableCompression)));
}

namespace UnitTest
{
	TEST_CLASS(SteganoPNGUnitTest)
	{
	public:
		
		TEST_METHOD(TestSHA256Generation)
		{
			std::string base = "Harambe";
			std::string sha256hash = "27504294D2547CF4D1D718F723ADD7FBE1B9626B2A1171AEB75B7D35F9E7E131";

			CryptoPP::byte* hash = SteganoPNG::generateSHA256(base);

			string ret;
			HexEncoder encoder;
			encoder.Attach(new CryptoPP::StringSink(ret));
			encoder.Put(hash, SHA256::DIGESTSIZE);
			encoder.MessageEnd();
			
			Assert::AreEqual(sha256hash, ret);
		}
		TEST_METHOD(TestEncryptionAndDecryption)
		{
			std::vector<unsigned char> plain = { 65, 116, 116, 97, 99, 107, 32, 97, 116, 32, 100, 97, 119, 110, 33, 65, 116, 116, 97, 99, 107, 32, 97, 116, 32, 100, 97, 119, 110, 33, 65, 116, 116, 97, 99, 107, 32, 97, 116, 32, 100, 97, 119, 110, 33 };

			CryptoPP::byte key[AES::MAX_KEYLENGTH];
			CryptoPP::byte iv[AES::BLOCKSIZE];

			std::string password = "Password";

			memcpy(key, SteganoPNG::generateSHA256(password), sizeof(key));
			memcpy(iv, key, sizeof(iv));

			vector<unsigned char> encrypted = SteganoPNG::Encrypt(key, iv, plain);

			memcpy(key, SteganoPNG::generateSHA256(password), sizeof(key));
			memcpy(iv, key, sizeof(iv));

			vector<unsigned char> decrypted = SteganoPNG::Decrypt(key, iv, encrypted);

			auto data1 = std::string(decrypted.begin(), decrypted.end());
			auto data2 = std::string(plain.begin(), plain.end());

			Assert::AreEqual(std::string(plain.begin(), plain.end()), std::string(decrypted.begin(), decrypted.end()));
			Assert::AreEqual(data1, data2);
			Assert::IsTrue(decrypted == plain, L"Unexpected value for <subsets>");

		}
		TEST_METHOD(TestCompressionAndDecompression)
		{

			string filename = std::string(TEST_CASE_DIRECTORY) + "lorem.txt";

			vector<unsigned char> plain = SteganoPNG::readAllBytes(filename);

#ifdef USEZOPFLI
			vector<CryptoPP::byte> compressed = SteganoPNG::zopfliCompress(plain);
#else
			vector<CryptoPP::byte> compressed = SteganoPNG::zlibCompress(plain);
#endif	

			try {
				vector<unsigned char> failed_compressed = SteganoPNG::zlibCompress(vector<unsigned char>(0));
			}
			catch (const std::runtime_error &ex) {
				Assert::AreEqual(ex.what(), "ZLib was not supplied with enough data");
			}

			vector<unsigned char> decompressed = SteganoPNG::zlibDecompress(compressed);

			Assert::AreEqual(std::string(plain.begin(), plain.end()), std::string(decompressed.begin(), decompressed.end()));
		}
		TEST_METHOD(TestIO)
		{
			std::string plain_string;
			for (int i = 0; i < 63; i++) {
				plain_string += "IOTest" + std::to_string(i) + "\n";
			}

			std::vector<CryptoPP::byte> plain_vector(plain_string.begin(), plain_string.end());

			SteganoPNG::writeAllBytes("TestIO.txt", plain_vector);

			std::vector<CryptoPP::byte> read_data = SteganoPNG::readAllBytes("TestIO.txt");

			remove("TestIO.txt");

			Assert::IsTrue(plain_vector == read_data);

		}
		TEST_METHOD(TestPNGEncodingAndDecoding)
		{
			std::vector<unsigned char> expected_image(1024, (unsigned char)255);

			string imagefile = std::string(TEST_CASE_DIRECTORY) + "test.png";
			Logger::WriteMessage(imagefile.c_str());

			std::vector<unsigned char> _image;
			unsigned int _width;
			unsigned int _height;
			lodepng::State _state;

			std::vector<CryptoPP::byte> raw_image = {	137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82,
														0, 0, 0, 16, 0, 0, 0, 16, 1, 0, 0, 0, 0, 55, 136, 194,
														204, 0, 0, 0, 22, 73, 68, 65, 84, 120, 1, 149, 194, 1, 13, 0,
														0, 0, 194, 32, 251, 151, 198, 12, 103, 140, 244, 253, 78, 31, 225, 247,
														41, 68, 93, 0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130 };

			SteganoPNG::writeAllBytes(imagefile, raw_image);
			SteganoPNG::decodeOneStep(imagefile.c_str(), &_image, &_width, &_height, &_state);
			remove(imagefile.c_str());

			SteganoPNG::encodeOneStep(imagefile.c_str(), _image, _width, _height, _state);
			std::vector<CryptoPP::byte> reconstructed_image = SteganoPNG::readAllBytes(imagefile);

			Assert::IsTrue(expected_image == _image);
			Assert::IsTrue(raw_image == reconstructed_image);
			remove(imagefile.c_str());

		}
		TEST_METHOD(TestNoiseGeneration)
		{
			vector<unsigned int> expected_noise{ 2348, 2410, 2408, 2459, 2395, 2406, 2449, 2397, 2458, 2416, 2383, 2341, 2377, 2453, 2347, 2376, 2375, 2364, 2426, 2345, 2429, 2414, 2362, 2415, 2349, 2412, 2387, 2338, 2450, 2413, 2365, 2455, 2447, 2366, 2435, 2423, 2361, 2339, 2428, 2442, 2462, 2381, 2393, 2440, 2444, 2358, 2360, 2432, 2391, 2425, 2368, 2443, 2371, 2399, 2373, 2403, 2356, 2392, 2401, 2342, 2389, 2457, 2386, 2372 };

			vector<unsigned int> noise = SteganoPNG::generateNoise(SteganoPNG::generateSHA256("Harambe"), (size_t)8, (size_t)(128+2336));

			Assert::IsTrue(expected_noise == noise);

		}
		TEST_METHOD(ValidateConfiguration01)
		{

			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest01.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest01.png";
			
			std::vector<unsigned char> plain_vector = InitializeTestFiles(testfile, imagefile);

			int argumentCount = 4;
			char* argumentValues[] = { "", "a", (char*)imagefile.c_str(), (char*)testfile.c_str() };

			ConfigurationManager config = ConfigurationManager(argumentCount, argumentValues);

			RunAsserts(config, ConfigurationManager::Mode::HIDE, imagefile, testfile, false, std::string(""), false);

			std::vector<unsigned char> extracted = HideAndExtract(config);

			Assert::IsTrue(plain_vector == extracted);

			remove(imagefile.c_str());

		}
		TEST_METHOD(ValidateConfiguration02)
		{

			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest02.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest02.png";

			std::vector<unsigned char> plain_vector = InitializeTestFiles(testfile, imagefile);

			int argumentCount = 6;
			char* argumentValues[] = { "", "a", (char*)imagefile.c_str(), (char*)testfile.c_str(), "-p", "Test" };

			ConfigurationManager config = ConfigurationManager(argumentCount, argumentValues);

			RunAsserts(config, ConfigurationManager::Mode::HIDE, imagefile, testfile, true, std::string("Test"), false);

			std::vector<unsigned char> extracted = HideAndExtract(config);

			Assert::IsTrue(plain_vector == extracted);

			remove(imagefile.c_str());

		}
		TEST_METHOD(ValidateConfiguration03)
		{

			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest03.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest03.png";

			std::vector<unsigned char> plain_vector = InitializeTestFiles(testfile, imagefile);

			int argumentCount = 5;
			char* argumentValues[] = { "", "a", (char*)imagefile.c_str(), (char*)testfile.c_str(), "--no-compression" };

			ConfigurationManager config = ConfigurationManager(argumentCount, argumentValues);

			RunAsserts(config, ConfigurationManager::Mode::HIDE, imagefile, testfile, false, std::string(""), true);

			std::vector<unsigned char> extracted = HideAndExtract(config);

			Assert::IsTrue(plain_vector == extracted);

			remove(imagefile.c_str());

		}
		TEST_METHOD(ValidateConfiguration04)
		{

			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest04.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest04.png";

			std::vector<unsigned char> plain_vector = InitializeTestFiles(testfile, imagefile);

			int argumentCount = 7;
			char* argumentValues[] = { "", "a", (char*)imagefile.c_str(), (char*)testfile.c_str(), "-p", "Test", "--no-compression" };

			ConfigurationManager config = ConfigurationManager(argumentCount, argumentValues);

			RunAsserts(config, ConfigurationManager::Mode::HIDE, imagefile, testfile, true, std::string("Test"), true);

			std::vector<unsigned char> extracted = HideAndExtract(config);

			Assert::IsTrue(plain_vector == extracted);

			remove(imagefile.c_str());

		}
		TEST_METHOD(ValidateConfiguration05)
		{

			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest05.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest05.png";

			std::vector<unsigned char> plain_vector = InitializeTestFiles(testfile, imagefile);

			int argumentCount = 3;
			char* argumentValues[] = { "", "x", (char*)imagefile.c_str() };

			ConfigurationManager config = ConfigurationManager(argumentCount, argumentValues);

			int argumentCount2 = 4;
			char* argumentValues2[] = { "", "a", (char*)imagefile.c_str(), (char*)testfile.c_str() };

			ConfigurationManager config2 = ConfigurationManager(argumentCount2, argumentValues2);

			RunAsserts(config, ConfigurationManager::Mode::EXTRACT, imagefile, std::string(""), false, std::string(""), false);

			HideOnly(config2);

			std::vector<unsigned char> extractedData = ExtractOnly(config);

			Assert::IsTrue(plain_vector == extractedData);

			remove(imagefile.c_str());

		}
		TEST_METHOD(ValidateConfiguration06)
		{

			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest06.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest06.png";

			std::vector<unsigned char> plain_vector = InitializeTestFiles(testfile, imagefile);

			int argumentCount = 5;
			char* argumentValues[] = { "", "x", (char*)imagefile.c_str(), "-p", "Test" };

			ConfigurationManager config = ConfigurationManager(argumentCount, argumentValues);

			int argumentCount2 = 6;
			char* argumentValues2[] = { "", "a", (char*)imagefile.c_str(), (char*)testfile.c_str(), "-p", "Test" };

			ConfigurationManager config2 = ConfigurationManager(argumentCount2, argumentValues2);

			RunAsserts(config, ConfigurationManager::Mode::EXTRACT, imagefile, std::string(""), true, std::string("Test"), false);

			HideOnly(config2);

			std::vector<unsigned char> extractedData = ExtractOnly(config);

			Assert::IsTrue(plain_vector == extractedData);

			remove(imagefile.c_str());

		}
		TEST_METHOD(ValidateConfiguration07)
		{

			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest07.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest07.png";

			std::vector<unsigned char> plain_vector = InitializeTestFiles(testfile, imagefile);

			int argumentCount = 4;
			char* argumentValues[] = { "", "x", (char*)imagefile.c_str(), "--no-compression" };

			ConfigurationManager config= ConfigurationManager(argumentCount, argumentValues);

			int argumentCount2 = 5;
			char* argumentValues2[] = { "", "a", (char*)imagefile.c_str(), (char*)testfile.c_str(), "--no-compression" };

			ConfigurationManager config2 = ConfigurationManager(argumentCount2, argumentValues2);

			RunAsserts(config, ConfigurationManager::Mode::EXTRACT, imagefile, std::string(""), false, std::string(""), true);

			HideOnly(config2);

			std::vector<unsigned char> extractedData = ExtractOnly(config);

			Assert::IsTrue(plain_vector == extractedData);

			remove(imagefile.c_str());

		}
		TEST_METHOD(ValidateConfiguration08)
		{

			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest08.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest08.png";

			std::vector<unsigned char> plain_vector = InitializeTestFiles(testfile, imagefile);

			int argumentCount = 6;
			char* argumentValues[] = { "", "x", (char*)imagefile.c_str(), "-p", "Test", "--no-compression" };

			ConfigurationManager config= ConfigurationManager(argumentCount, argumentValues);

			int argumentCount2 = 7;
			char* argumentValues2[] = { "", "a", (char*)imagefile.c_str(), (char*)testfile.c_str(), "-p", "Test", "--no-compression" };

			ConfigurationManager config2 = ConfigurationManager(argumentCount2, argumentValues2);

			RunAsserts(config, ConfigurationManager::Mode::EXTRACT, imagefile, std::string(""), true, std::string("Test"), true);

			HideOnly(config2);

			std::vector<unsigned char> extractedData = ExtractOnly(config);

			Assert::IsTrue(plain_vector == extractedData);

			remove(imagefile.c_str());

		}
		TEST_METHOD(ValidateConfiguration09)
		{
			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest09.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest09.png";

			InitializeTestFiles(testfile, imagefile);

			int argumentCount = 4;
			char* argumentValues[] = { "", "t", (char*)imagefile.c_str(), (char*)testfile.c_str() };

			ConfigurationManager config= ConfigurationManager(argumentCount, argumentValues);

			RunAsserts(config, ConfigurationManager::Mode::TEST, imagefile, testfile, false, std::string(""), false);

			Assert::IsTrue(ValidateStorageSpace(config));

			remove(imagefile.c_str());
			remove(testfile.c_str());

		}
		TEST_METHOD(ValidateConfiguration10)
		{
			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest10.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest10.png";

			InitializeTestFiles(testfile, imagefile);

			int argumentCount = 6;
			char* argumentValues[] = { "", "t", (char*)imagefile.c_str(), (char*)testfile.c_str(), "-p", "Test" };

			ConfigurationManager config= ConfigurationManager(argumentCount, argumentValues);

			RunAsserts(config, ConfigurationManager::Mode::TEST, imagefile, testfile, true, std::string("Test"), false);

			Assert::IsTrue(ValidateStorageSpace(config));

			remove(imagefile.c_str());
			remove(testfile.c_str());

		}
		TEST_METHOD(ValidateConfiguration11)
		{
			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest11.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest11.png";

			InitializeTestFiles(testfile, imagefile);

			int argumentCount = 5;
			char* argumentValues[] = { "", "t", (char*)imagefile.c_str(), (char*)testfile.c_str(), "--no-compression" };

			ConfigurationManager config= ConfigurationManager(argumentCount, argumentValues);

			RunAsserts(config, ConfigurationManager::Mode::TEST, imagefile, testfile, false, std::string(""), true);

			Assert::IsTrue(ValidateStorageSpace(config));

			remove(imagefile.c_str());
			remove(testfile.c_str());

		}
		TEST_METHOD(ValidateConfiguration12)
		{
			string testfile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest12.txt";
			string imagefile = std::string(TEST_CASE_DIRECTORY) + "ConfigurationTest12.png";

			InitializeTestFiles(testfile, imagefile);

			int argumentCount = 7;
			char* argumentValues[] = { "", "t", (char*)imagefile.c_str(), (char*)testfile.c_str(), "-p", "Test", "--no-compression" };

			ConfigurationManager config = ConfigurationManager(argumentCount, argumentValues);

			RunAsserts(config, ConfigurationManager::Mode::TEST, imagefile, testfile, true, std::string("Test"), true);

			Assert::IsTrue(ValidateStorageSpace(config));

			remove(imagefile.c_str());
			remove(testfile.c_str());

		}
		TEST_METHOD(ValidateConfiguration13)
		{
			int argumentCount = 2;
			char* argumentValues[] = { "", "h" };

			ConfigurationManager config = ConfigurationManager(argumentCount, argumentValues);

			RunAsserts(config, ConfigurationManager::Mode::HELP, std::string(""), std::string(""), false, std::string(""), false);

		}
		TEST_METHOD(ValidateConfiguration14)
		{
			int argumentCount = 2;
			char* argumentValues[] = { "", "b" };

			ConfigurationManager config = ConfigurationManager(argumentCount, argumentValues);

			RunAsserts(config, ConfigurationManager::Mode::HELP, std::string(""), std::string(""), false, std::string(""), false);
		}
	};
}
