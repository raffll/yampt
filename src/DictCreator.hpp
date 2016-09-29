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

	std::string getName() { return esm_n.getName(); }
	std::string getNamePrefix() { return esm_n.getNamePrefix(); }
	yampt::dict_t const& getDict() const { return dict; }

	DictCreator(std::string path_n);
	DictCreator(std::string path_n, std::string path_f);
	DictCreator(std::string path_n, DictMerger &m, bool no_duplicates);

private:
	void compareEsm();
	void resetCounters();
	std::string dialTranslator(std::string to_translate);
	void validateRecord(const std::string &unique_key, const std::string &friendly, yampt::r_type type, bool extra = false);
	void insertRecord(const std::string &unique_key, const std::string &friendly, yampt::r_type type, bool extra);
	std::vector<std::string> makeMessageColl(const std::string &script_text);
	void printLog(std::string id, bool header = false);

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

	int counter_inserted;
	int counter_cell;
	int counter_skipped;
	int counter_all;

	yampt::dict_t dict;

	std::vector<std::string> *message_ptr;
	std::vector<std::string> message_n;
	std::vector<std::string> message_f;
};

#endif
