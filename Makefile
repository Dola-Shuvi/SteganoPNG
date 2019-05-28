CXX=g++
CXXFLAGS=-O2 -o Steganography

all: install

install: Steganography
	cd Steganography; \
	$(CXX) $(CXXFLAGS) Steganography.cpp lodepng.cpp; \
	cd ..; \
	mkdir build; \
	mv Steganography/Steganography build/Steganography
	
clean:
	rm -rf build/* \
	rm Steganography/Steganography