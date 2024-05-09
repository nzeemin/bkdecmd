
CXXFLAGS = -std=c++17 -Wall -g -O0

SOURCES_IMGOS = $(foreach dir, ./imgos, $(wildcard $(dir)/*.cpp))
SOURCES = pch.cpp bkdecmd.cpp BKImage.cpp BKImgFile.cpp BKParseImage.cpp StringUtil.cpp $(SOURCES_IMGOS)

OBJECTS = $(patsubst %.cpp, %.o, $(SOURCES))

all: bkdecmd

bkdecmd: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o bkdecmd $(OBJECTS)

.PHONY: clean

clean:
	rm -f $(OBJECTS)
