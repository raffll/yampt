#ifndef CREATOR_HPP
#define CREATOR_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <array>
#include <map>

#include "tools.hpp"
#include "esmtools.hpp"

using namespace std;

class creator : tools
{
public:

	void writeDictAll();
	void writeDict(int i);
	void printDict(int i);

	creator();
	creator(const char* b);
	creator(const char* b, const char* e);

private:
	void printStatus(int i);

	void makeDictCell();
	void makeDictGmst();
	void makeDictFnam();
	void makeDictDesc();
	void makeDictBook();
	void makeDictFact();
	void makeDictIndx();
	void makeDictDial();
	void makeDictInfo();
	void makeDictScpt();

	esmtools base;
	esmtools extd;
	esmtools *esm_ptr;
	int counter;
	array<multimap<string, string>, 10> dict;
};

#endif
