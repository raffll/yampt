CC=g++
CFLAGS=-c -std=c++11 -Wall -Wextra -O2 -pedantic -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2
LDFLAGS=
SOURCES=main.cpp esmtools.cpp creator.cpp dicttools.cpp tools.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=yampt

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
