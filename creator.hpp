#ifndef CREATOR_HPP
#define CREATOR_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <array>
#include <map>
#include <algorithm>

#include "tools.hpp"
#include "esmtools.hpp"
#include "merger.hpp"

using namespace std;

class creator
{
public:
	void makeDict();
	void writeDict();
	void writeScripts();
	void compareEsm();
	void eraseDuplicates();
	void eraseDifferent();

	creator() {}
	creator(string esm_path);
	creator(string esm_path, string ext_path);
	creator(string esm_path, merger &m, bool no_dupl = 0);

private:
	string dialTranslator(string to_translate);
	string makeGap(string str);
	void insertRecord(const string &pri, const string &sec);
	void makeDictCELL();
	void makeDictGMST();
	void makeDictFNAM();
	void makeDictDESC();
	void makeDictTEXT();
	void makeDictRNAM();
	void makeDictINDX();
	void makeDictDIAL();
	void makeDictINFO();
	void makeDictBNAM();
	void makeDictSCPT();

	esmtools esm;
	esmtools ext;
	esmtools *esm_ptr;
	merger dict;
	bool status = 0;
	bool with_dict = 0;
	bool no_duplicates = 0;
	int counter;
	map<string, string> created;
};

#endif
