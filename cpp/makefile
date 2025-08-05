CXX = g++
CXXFLAGS = -g3 -O3 -Wall -std=c++23

all: Objparser 

Objparser: objparser.o mtlparser.o
	$(CXX) $(CXXFLAGS) -o $@ $^

objparser.o: objparser.hpp mtlparser.hpp

clean:
	rm Objparser objparser.o Mtlparser mtlparser.o
