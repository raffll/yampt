#ifndef DICTCREATOR_HPP
#define DICTCREATOR_HPP

#include "Config.hpp"
#include "EsmReader.hpp"
#include "EsmRecord.hpp"
#include "DictMerger.hpp"

class DictCreator : public Tools
{
public:
	void makeDict();
	void makeScriptText();
	void compareEsm();

	std::string getName() { return esm_n.getNamePrefix(); }
	yampt::dict_t const& getDict() const { return dict; }
	std::string getScriptText() { return script_text; }

	DictCreator(std::string path_n, bool replace_broken);
	DictCreator(std::string path_n, std::string path_f, bool replace_broken);
	DictCreator(std::string path_n, DictMerger &m, bool no_duplicates, bool replace_broken);

private:
	std::string dialTranslator(std::string to_translate);
	void insertRecord(const std::string &pri_text,
			  const std::string &sec_text,
			  yampt::r_type type,
			  bool extra = false);
	std::vector<std::string> makeMessageColl(const std::string &script_text);

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
	DictMerger *merger;

	bool status = 0;
	bool with_dict = 0;
	bool no_duplicates = 0;

	int counter;
	int counter_cell;

	yampt::dict_t dict;
	std::string script_text;

	std::vector<std::string> *message_ptr;
	std::vector<std::string> message_n;
	std::vector<std::string> message_f;
};

#endif
