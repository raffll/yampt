#ifndef ESMCONVERTER_HPP
#define ESMCONVERTER_HPP

#include "Config.hpp"
#include "EsmReader.hpp"
#include "EsmRecord.hpp"
#include "DictMerger.hpp"

using namespace std;

class EsmConverter
{
public:
	void convertEsm();
	void convertEsmWithDial();
	void writeEsm();
	bool getStatus() { return status; }

	EsmConverter();
	EsmConverter(string path, DictMerger &m);

private:
	string convertIntToByteArray(unsigned int x);
	bool caseInsensitiveStringCmp(string lhs, string rhs);
	void convertRecordContent(size_t pos, size_t old_size, string new_text,
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

	bool status;
	EsmRecord esm;
	DictMerger merger;
	int counter;
	string rec_content;
	string script_text;
};

#endif
