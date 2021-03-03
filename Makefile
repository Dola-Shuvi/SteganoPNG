CXX=g++
CXXFLAGS=-O3 -static -std=c++17 -ICryptoPP -ILodePNG -Izopfli/src/zopfli/ -Lzopfli -LCryptoPP -o SteganoPNG
LDFLAGS=-Wl,-flto -Wl,-O2
LDLIBS=-lstdc++fs -l:libcryptopp.a
ZOPFLI=

all: install

install: cryptopp zopfli
	cd Steganography; \
	$(CXX) $(CXXFLAGS) SteganoPNG.cpp ConfigurationManager.cpp LodePNG/lodepng.cpp $(LDLIBS) $(ZOPFLI); \
	strip SteganoPNG; \
	cd ..; \
	mkdir -p build; \
	mv Steganography/SteganoPNG build/SteganoPNG

zopfli:
ifneq (,$(wildcard ./Steganography/zopfli/libzopfli.a))
ZOPFLI := -l:libzopfli.a
endif

cryptopp:
ifeq (,$(wildcard ./Steganography/CryptoPP/libcryptopp.a))	
	cd Steganography/CryptoPP/; \
	make
endif

clean:
	rm -rf build/* \
	rm Steganography/SteganoPNG
