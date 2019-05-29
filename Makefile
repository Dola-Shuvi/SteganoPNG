CXX=g++
CXXFLAGS=-O2 -o Steganography

all: install

install: lodepng
	cd Steganography; \
	$(CXX) $(CXXFLAGS) Steganography.cpp lodepng.cpp; \
	cd ..; \
	mkdir -p build; \
	mv Steganography/Steganography build/Steganography
	
lodepng:
	cd Steganography; \
	wget -q -N https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.h https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.cpp
	
clean:
	rm -rf build/* \
	rm Steganography/Steganography