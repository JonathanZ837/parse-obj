CXX = g++
CXXFLAGS = -g3 -O3 -Wall -std=c++23

all: Objparser Objparser_m

Objparser: objparser.o
	$(CXX) $(CXXFLAGS) -o $@ $^
Objparser_m: objparser_mmap.o
	$(CXX) $(CXXFLAGS) -o $@ $^

objparser.o: objparser.hpp
objparser_mmap.o: objparser_mmap.hpp
clean:
	rm Objparser objparser.o Objparser_m objparser_mmap.o