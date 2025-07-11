CXX = g++
CXXFLAGS = -g3 -Wall -std=c++23

all: Objparser

Objparser: objparser.o
	$(CXX) $(CXXFLAGS) -o $@ $^

objparser.o: objparser.hpp

clean:
	rm Objparser objparser.o