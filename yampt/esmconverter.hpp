#ifndef ESMCONVERTER_HPP
#define ESMCONVERTER_HPP

#include "includes.hpp"
#include "tools.hpp"
#include "esmreader.hpp"
#include "dictmerger.hpp"

class EsmConverter
{
public:
	bool isLoaded() { return esm.isLoaded(); }
	std::string getNameFull() { return esm.getNameFull(); }
	std::string getNamePrefix() { return esm.getNamePrefix(); }
	std::string getNameSuffix() { return esm.getNameSuffix(); }
	std::time_t getTime() { return esm.getTime(); }
	std::vector<std::string> getRecordColl() { return esm.getRecords(); }

	EsmConverter(
		const std::string & path,
		const DictMerger & merger,
		const bool add_hyperlinks,
		const bool safe,
		const std::string & file_suffix,
		const Tools::Encoding encoding);

private:
	void convertEsm(const bool safe);
	void resetCounters();
	void convertRecordContent();
	void addNullTerminatorIfEmpty();
	void setNewText(const std::string & prefix = "");
	void checkIfIdentical();
	void printLogLine(const Tools::RecType type);
	void convertMAST();
	void convertCELL();
	void convertPGRD();
	void convertANAM();
	void convertSCVR();
	void convertDNAM();
	void convertCNDT();
	void convertGMST();
	void convertFNAM();
	void convertDESC();
	void convertTEXT();
	void convertRNAM();
	void convertINDX();
	void convertDIAL();
	void convertINFO();
	void convertBNAM();
	void convertSCPT();

	EsmReader esm;
	const DictMerger * merger;

	bool add_hyperlinks;
	const std::string file_suffix;

	int counter_converted = 0;
	int counter_identical = 0;
	int counter_unchanged = 0;
	int counter_all = 0;
	int counter_added = 0;

	bool ready = false;

	Tools::Encoding esm_encoding = Tools::Encoding::UNKNOWN;

	std::string key_text;
	std::string val_text;
	Tools::RecType type;

	std::string new_text;
};

#endif // ESMCONVERTER_HPP
