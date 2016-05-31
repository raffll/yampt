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

	creator() {}
	creator(string esm_path);
	creator(string esm_path, string ext_path);

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
	void makeDictBnam();
	void makeDictScpt();

	esmtools esm;
	esmtools ext;
	esmtools *esm_ptr;
	int counter;
	map<string, string> dict;
};

#endif
