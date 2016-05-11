#ifndef CREATOR_HPP
#define CREATOR_HPP

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <map>

#include "tools.hpp"

using namespace std;

class creator : public tools
{
public:
	esmtools base;
	esmtools extd;
	esmtools *esm_ptr;
	bool extd_switch;
	array<multimap<string, string>, 10> dict;
	vector<string> key = {"Choice", "choice", "MessageBox", "Say ", "Say,", "say ", "say,"};
	string sep = {"^"};

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

	void makeDictAll();
	void writeDictAll();
	void writeDict(int i);
	void printDict(int i);
	string extdText();

	creator();
	creator(const char* b);
	creator(const char* b, const char* e, bool x = 0);
};

#endif
