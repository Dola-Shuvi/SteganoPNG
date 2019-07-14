CXX=g++
CXXFLAGS=-O3 -static -std=c++17 -I/usr/local/include/cryptopp -ILodePNG/ -o SteganoPNG
LDFLAGS=-Wl,-flto -Wl,-O2
LDLIBS=-lstdc++fs -l:libcryptopp.a

all: install

install: lodepng
	cd Steganography; \
	$(CXX) $(CXXFLAGS) SteganoPNG.cpp LodePNG/lodepng.cpp $(LDLIBS); \
	strip SteganoPNG; \
	cd ..; \
	mkdir -p build; \
	mv Steganography/SteganoPNG build/SteganoPNG
	
lodepng:
	cd Steganography/LodePNG; \
	wget -q -N https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.h https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.cpp
	
clean:
	rm -rf build/* \
	rm Steganography/SteganoPNG
