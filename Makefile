CXX=g++
CXXFLAGS=-O2 -std=c++17 -o SteganoPNG
LDLIBS=-lstdc++fs

all: install

install: lodepng
	cd Steganography; \
	$(CXX) $(CXXFLAGS) SteganoPNG.cpp lodepng.cpp $(LDLIBS); \
	cd ..; \
	mkdir -p build; \
	mv Steganography/SteganoPNG build/SteganoPNG
	
lodepng:
	cd Steganography; \
	wget -q -N https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.h https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.cpp
	
clean:
	rm -rf build/* \
	rm Steganography/SteganoPNG
