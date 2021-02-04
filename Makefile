CXX=g++
CXXFLAGS=-O3 -static -std=c++17 -ICryptoPP -ILodePNG -Izopfli/src/zopfli/ -Lzopfli -LCryptoPP -o SteganoPNG
LDFLAGS=-Wl,-flto -Wl,-O2
LDLIBS=-lstdc++fs -l:libcryptopp.a
ZOPFLI=

all: install

install: lodepng cryptopp zopfli
	cd Steganography; \
	$(CXX) $(CXXFLAGS) SteganoPNG.cpp LodePNG/lodepng.cpp $(LDLIBS) $(ZOPFLI); \
	strip SteganoPNG; \
	cd ..; \
	mkdir -p build; \
	mv Steganography/SteganoPNG build/SteganoPNG
	
lodepng:
	cd Steganography/LodePNG; \
	wget -q -N https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.h https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.cpp
	
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
