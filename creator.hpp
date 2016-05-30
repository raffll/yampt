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

class creator
{
public:
	void makeDict();
	void writeDict();

	dict_t const& getDict(int i) const { return dict[i]; }

	creator() {}
	creator(string path_base);
	creator(string path_base, string path_extd);

private:
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
	int rec_counter;
	array<dict_t, 10> dict;
};

#endif
