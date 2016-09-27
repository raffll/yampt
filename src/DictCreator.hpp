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

	std::string getName() { return esm_n.getNamePrefix(); }
	yampt::dict_t const& getDict() const { return dict; }

	DictCreator(std::string path_n);
	DictCreator(std::string path_n, std::string path_f);
	DictCreator(std::string path_n, DictMerger &m, bool no_duplicates);

private:
	void compareEsm();
	std::string dialTranslator(std::string to_translate);
	void insertRecord(const std::string &unique_key,
			  const std::string &friendly,
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

	bool status = false;
	bool with_dict = false;
	bool no_duplicates = false;

	int counter;
	int counter_cell;

	yampt::dict_t dict;

	std::vector<std::string> *message_ptr;
	std::vector<std::string> message_n;
	std::vector<std::string> message_f;
};

#endif
