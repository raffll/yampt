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

using namespace std;

class creator : public tools
{
public:
	esmtools base;
	esmtools extd;

	void writeDictAll();
	void writeDict(int i);
	void printDict(int i);

	creator();
	creator(const char* b);
	creator(const char* b, const char* e);

private:
	esmtools *esm_ptr;
	array<multimap<string, string>, 10> dict;

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
};

#endif
