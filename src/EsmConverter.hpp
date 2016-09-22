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

	bool getStatus() { return status; }

	EsmConverter();
	EsmConverter(std::string path, DictMerger &n);

private:
	void setConditions(const std::string &pri_text_n,
			   const std::string &pri_text_f,
			   const std::string &sec_text,
			   RecType type);
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
	void convertBNAM();
	void convertSCPT();

	bool status = 0;
	EsmRecord esm;
	DictMerger merger;
	int counter = 0;
	int counter_safe = 0;
	int counter_add = 0;
	int counter_message = 0;
	int counter_dial = 0;
	int counter_cell = 0;
	std::string rec_content;
	std::string script_text;

	bool found_n = false;
	bool found_f = false;
	bool equal_n = false;
	bool equal_f = false;
};

#endif
