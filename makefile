CC=g++
CFLAGS=-c -std=c++11 -Wall -Wextra -O2 -pedantic -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2
LDFLAGS=
SOURCES=Main.cpp EsmTools.cpp RecTools.cpp DictCreator.cpp DictTools.cpp DictMerger.cpp EsmConverter.cpp Config.cpp UserInterface.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=yampt

.PHONY: clean all

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o *.gch *.log *.dic *.esp *.esm
