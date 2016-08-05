#ifndef CREATOR_HPP
#define CREATOR_HPP

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
	void compareEsm();

	Creator() {}
	Creator(std::string esm_path);
	Creator(std::string esm_path, std::string ext_path);
	Creator(std::string esm_path, Merger &m, bool no_dupl = 0);

private:
	string dialTranslator(string to_translate);
	string makeGap(string str);
	void insertRecord(const std::string &pri, const std::string &sec);
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
	std::map<std::string, std::string> created;
};

#endif
