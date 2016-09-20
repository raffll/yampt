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
	void convertEsmWithDIAL();
	void convertEsmSafe();
	void convertEsmStats();
	void writeEsm();
	bool getStatus() { return status; }

	EsmConverter();
	EsmConverter(std::string path, DictMerger &m);

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

	void convertStatsARMO();
	void convertStatsMGEF();
	void convertStatsMISC();
	void convertStatsWEAP();
	void convertStatsCLOT();

	bool status = 0;
	EsmRecord esm;
	DictMerger merger;
	int counter = 0;
	bool safe = 0;
	std::string rec_content;
	std::string script_text;
};

#endif
