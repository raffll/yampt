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
	std::string getName() { return esm.getNamePrefix(); }
	std::string getLog() { return log; }

	EsmConverter(std::string path, DictMerger &n, bool convert_safe, bool add_dial);

private:
	void resetCounters();
	void convertRecordContent(std::string new_text);

	void setNewFriendly(yampt::r_type type);
	void setNewFriendlyINFO(yampt::r_type type);
	void setNewFriendlyScript(std::string id, yampt::r_type type);

	void convertLine(std::string id, yampt::r_type type);
	void convertText(std::string id, yampt::r_type type, int num);
	void extractText(int num);
	std::vector<std::string> splitLine(std::string line);

	void addDIALtoINFO();

	void makeLog(std::string key);
	void makeLogScript(std::string key);
	void printLog(std::string id, bool header = false);

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
	void convertGMDT();

	bool status = false;
	EsmRecord esm;
	DictMerger *merger;

	int counter_converted = 0;
	int counter_unchanged = 0;
	int counter_skipped = 0;
	int counter_all = 0;
	int counter_add = 0;

	std::string unique_key;
	std::string new_friendly;
	std::string current_dialog;
	std::string compiled;

	std::string log;
	const std::string *result_ptr;

	bool found_key = false;
	bool convert = false;
	bool safe = false;
	bool add_dial = false;

	std::string line;
	std::string line_new;
	std::string text;
	size_t pos;
	size_t pos_c;
	int num;
};

#endif
