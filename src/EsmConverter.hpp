#ifndef ESMCONVERTER_HPP
#define ESMCONVERTER_HPP

#include "Config.hpp"
#include "EsmReader.hpp"
#include "EsmRecord.hpp"
#include "DictMerger.hpp"

class EsmConverter : public Tools
{
public:
	void convertEsm();
	void writeEsm();

	bool getStatus() { return status; }
	std::string getLog() { return log_detailed; }

	EsmConverter();
	EsmConverter(std::string path, DictMerger &n);

private:
	void resetCounters();
	void setNewFriendly(RecType type);
	void setSafeConditions(RecType type);
	void convertRecordContent(std::string new_text);
	void convertScript(RecType type, std::string id);
	void extractText(const std::string &line, std::string &text, size_t &pos);
	void addDIALtoINFO();
	void makeDetailedLog(std::string id, bool safe = false);
	void printLog(std::string id);

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

	bool status = 0;
	EsmRecord esm;
	DictMerger merger;

	int counter = 0;

	std::string unique_n;
	std::string unique_f;
	std::string new_friendly;
	std::string new_script;

	std::string log_detailed;
	std::string result;

	bool found_n = false;
	bool found_f = false;
	bool equal_n = false;
	bool equal_f = false;
};

#endif
