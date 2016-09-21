#ifndef ESMCONVERTER_HPP
#define ESMCONVERTER_HPP

#include "Config.hpp"
#include "EsmReader.hpp"
#include "EsmRecord.hpp"
#include "DictMerger.hpp"

class EsmConverter
{
public:
	void convertEsm();
	void writeEsm();

	void setSafeConvert(bool x) { safe_convert = x; }
	void setAddDialToInfo(bool x) { add_dial_to_info = x; }

	bool getStatus() { return status; }

	EsmConverter();
	EsmConverter(std::string path, DictMerger &n, DictMerger &f);

private:
	std::string convertIntToByteArray(unsigned int x);
	bool caseInsensitiveStringCmp(std::string lhs, std::string rhs);
	void convertRecordContent(size_t pos, size_t old_size, std::string new_text,
				  size_t new_size);
	void convertScriptLine(size_t i);
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
	void convertINFOWithDIAL();
	void convertBNAM();
	void convertSCPT();

	bool status = 0;
	EsmRecord esm;
	DictMerger merger_n;
	DictMerger merger_f;
	int counter = 0;
	int counter_safe = 0;
	int counter_add = 0;
	int counter_message = 0;
	int counter_dial = 0;
	int counter_cell = 0;
	bool safe_convert = 0;
	bool add_dial_to_info = 0;
	std::string rec_content;
	std::string script_text;
};

#endif
