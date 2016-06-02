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
#include "merger.hpp"

using namespace std;

class creator
{
public:
	void makeDict();
	void writeDict(bool after_convertion = 0);
	void compareEsm();
	void eraseDuplicates();
	void eraseDifferent();

	creator() {}
	creator(string esm_path);
	creator(string esm_path, string ext_path);
	creator(string esm_path, merger &m);

private:
	string dialTranslator(string to_translate);
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
	merger dict;
	bool status = {};
	bool with_dict = {};
	int counter;
	map<string, string> created;
};

#endif
