CXX=g++
CXXFLAGS=-O3 -static -std=c++17 -ICryptoPP -ILodePNG -LCryptoPP
LDFLAGS=-Wl,-flto -Wl,-O2
LDLIBS=-lstdc++fs -l:libcryptopp.a
CPPFLAGS=
LIBZOPFLI=
INCLUDEZOPFLI=

all: install

install: cryptopp zopfli
	cd Steganography; \
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDEZOPFLI) -o SteganoPNG SteganoPNG.cpp ConfigurationManager.cpp LodePNG/lodepng.cpp $(LDLIBS) $(LIBZOPFLI); \
	strip SteganoPNG; \
	cd ..; \
	mkdir -p build; \
	mv Steganography/SteganoPNG build/SteganoPNG

zopfli:
ifeq (-DUSEZOPFLI,$(CPPFLAGS))
ifeq (,$(wildcard ./Steganography/zopfli/libzopfli.a))
	cd Steganography/zopfli/; \
	cmake .; \
	cmake --build .
endif
LIBZOPFLI := -l:libzopfli.a
INCLUDEZOPFLI := -Izopfli/src/zopfli/ -Lzopfli
endif

cryptopp:
ifeq (,$(wildcard ./Steganography/CryptoPP/libcryptopp.a))	
	cd Steganography/CryptoPP/; \
	make
endif

clean:
	rm -rf build/* \
	rm Steganography/SteganoPNG
