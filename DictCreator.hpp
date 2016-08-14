#ifndef DICTCREATOR_HPP
#define DICTCREATOR_HPP

#include "Config.hpp"
#include "EsmTools.hpp"
#include "RecTools.hpp"
#include "DictMerger.hpp"

using namespace std;

class DictCreator
{
public:
	void makeDict();
	void writeDict();
	void writeScripts();
	void compareEsm();

	DictCreator() {}
	DictCreator(string esm_path);
	DictCreator(string esm_path, string ext_path);
	DictCreator(string esm_path, DictMerger &m, bool no_dupl = 0);

private:
	string dialTranslator(string to_translate);
	string makeGap(string str);
	void insertRecord(const string &pri_text, const string &sec_text, int dict_num);
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

	RecTools esm_n;
	RecTools esm_f;
	RecTools *esm_ptr;
	DictMerger dict_merged;
	bool status = 0;
	bool with_dict = 0;
	bool no_duplicates = 0;
	int counter;
	map<string, string> dict_created;
};

#endif
