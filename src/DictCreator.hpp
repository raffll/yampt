#ifndef DICTCREATOR_HPP
#define DICTCREATOR_HPP

#include "Config.hpp"
#include "EsmReader.hpp"
#include "EsmRecord.hpp"
#include "DictMerger.hpp"

using namespace std;

class DictCreator
{
public:
	void makeDict();
	void writeScripts();
	void compareEsm();
	void setNoDuplicates() { no_duplicates = 1; }

	string getName() { return esm_n.getNamePrefix(); }
	array<map<string, string>, 11> const& getDict() const { return dict; }

	DictCreator();
	DictCreator(string path_n);
	DictCreator(string path_n, string path_f);
	DictCreator(string path_n, DictMerger &m);

private:
	string dialTranslator(string to_translate);
	void insertRecord(const string &pri_text, const string &sec_text, RecType type, bool extra = 0);
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

	EsmRecord esm_n;
	EsmRecord esm_f;
	EsmRecord *esm_ptr;
	DictMerger merger;
	bool status = 0;
	bool with_dict = 0;
	bool no_duplicates = 0;
	int counter;
	int counter_cell;
	array<map<string, string>, 11> dict;
};

#endif
