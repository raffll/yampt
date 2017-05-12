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
	void binaryDump();

	std::string getName() { return esm_n.getName(); }
	std::string getNamePrefix() { return esm_n.getNamePrefix(); }
	yampt::dict_t const& getDict() const { return dict; }
	std::string getBinaryDump() { return dump; }

	DictCreator(std::string path_n);
	DictCreator(std::string path_n, std::string path_f);
	DictCreator(std::string path_n, DictMerger &m, yampt::ins_mode mode);

private:
	void makeDictBasic();
	void makeDictExtended();
	bool compareMasterFiles();
	void resetCounters();
	std::string dialTranslator(std::string to_translate);
	void validateRecord(const std::string &unique_key,
			    const std::string &friendly,
			    yampt::r_type type);
	void insertRecord(const std::string &unique_key,
			  const std::string &friendly,
			  yampt::r_type type);
	void makeLog(const std::string unique_key, const std::string friendly);
	std::vector<std::string> makeMessageColl(const std::string &script_text);
	void printLog(std::string id);
	void printLogHeader();

	void makeDictCELL();
	void makeDictCELLExtended();
	void makeDictDefaultCELL();
	void makeDictDefaultCELLExtended();
	void makeDictRegionCELL();
	void makeDictRegionCELLExtended();
	void makeDictGMST();
	void makeDictFNAM();
	void makeDictDESC();
	void makeDictTEXT();
	void makeDictRNAM();
	void makeDictINDX();
	void makeDictDIAL();
	void makeDictDIALExtended();
	void makeDictINFO();
	void makeDictBNAM();
	void makeDictBNAMExtended();
	void makeDictSCPT();
	void makeDictSCPTExtended();

	EsmRecord esm_n;
	EsmRecord esm_f;
	EsmRecord *esm_ptr;
	DictMerger *merger;

	bool status = false;
	bool basic_mode = false;

	yampt::ins_mode mode;

	int counter_created;
	int counter_doubled;
	int counter_identical;
	int counter_all;

	yampt::dict_t dict;

	std::vector<std::string> *message_ptr;
	std::vector<std::string> message_n;
	std::vector<std::string> message_f;

	std::string dump;
};

#endif
