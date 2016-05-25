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

class creator : public tools
{
public:
	creator();
	creator(const char* path_base);
	creator(const char* path_base, const char* path_extd);

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
	array<dict_t, 10> dict;
};

#endif
