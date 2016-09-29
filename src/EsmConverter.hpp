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
	void convertText(std::string id, yampt::r_type type);
	void extractText();

	void addDIALtoINFO();

	void makeLog(std::string id);
	void makeLogScript();
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

	bool status = false;
	EsmRecord esm;
	DictMerger *merger;

	int counter_converted = 0;
	int counter_notfound = 0;
	int counter_skipped = 0;
	int counter_all = 0;
	int counter_add = 0;

	std::string unique_key;
	std::string new_friendly;
	std::string current_dialog;

	std::string log;
	const std::string *result_ptr;

	bool convert = false;
	bool safe = false;
	bool add_dial = false;

	bool s_found = false;
	std::string s_line;
	std::string s_line_lc;
	std::string s_line_new;
	std::string s_text;
	size_t s_pos = 0;
};

#endif
