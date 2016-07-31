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
#include <locale>

#include "config.hpp"
#include "esmtools.hpp"
#include "merger.hpp"

using namespace std;

class Creator
{
public:
	void makeDict();
	void writeDict();
	void writeScripts();
	void writeBinary();
	void compareEsm();
	void eraseDuplicates();
	void eraseDifferent();

	Creator() {}
	Creator(string esm_path);
	Creator(string esm_path, string ext_path);
	Creator(string esm_path, Merger &m, bool no_dupl = 0);

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

	Esmtools esm;
	Esmtools ext;
	Esmtools *esm_ptr;
	Merger dict;
	bool status = 0;
	bool with_dict = 0;
	bool no_duplicates = 0;
	int counter;
	map<string, string> created;
};

#endif
