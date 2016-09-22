#ifndef DICTCREATOR_HPP
#define DICTCREATOR_HPP

#include "Config.hpp"
#include "EsmReader.hpp"
#include "EsmRecord.hpp"
#include "DictMerger.hpp"

class DictCreator
{
public:
	void makeDict();
	void makeScriptText();
	void compareEsm();
	void setNoDuplicates(bool x) { no_duplicates = x; }

	std::string getName() { return esm_n.getNamePrefix(); }
	yampt::dict_t const& getDict() const { return dict; }
	std::string getScriptText() { return raw_text; }

	DictCreator();
	DictCreator(std::string path_n);
	DictCreator(std::string path_n, std::string path_f);
	DictCreator(std::string path_n, DictMerger &m);

private:
	std::string dialTranslator(std::string to_translate);
	void insertRecord(const std::string &pri_text,
			  const std::string &sec_text,
			  RecType type,
			  bool extra = false);
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
	yampt::dict_t dict;
	std::string raw_text;
};

#endif
