SOURCES=Main.cpp EsmTools.cpp RecTools.cpp DictCreator.cpp DictTools.cpp DictMerger.cpp EsmConverter.cpp Config.cpp UserInterface.cpp
EXECUTABLE=yampt

CFLAGS=-c -std=c++11 -Wall -Wextra -O2 -pedantic -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2
LDFLAGS=
OBJECTS_DIR=obj/
SOURCES_DIR=src/
BIN_DIR=bin/

CC=g++
RM=rm

OBJECTS=$(SOURCES:.cpp=.o)
CSOURCES=$(addprefix $(SOURCES_DIR),$(SOURCES))
COBJECTS=$(addprefix $(OBJECTS_DIR),$(OBJECTS))
CEXECUTABLE=$(addprefix $(BIN_DIR),$(EXECUTABLE))

all: $(CSOURCES) $(CEXECUTABLE)

$(CEXECUTABLE): $(COBJECTS)
	$(CC) $(LDFLAGS) $(COBJECTS) -o $@

$(OBJECTS_DIR)%.o: $(SOURCES_DIR)%.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(COBJECTS)
