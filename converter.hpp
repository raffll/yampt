#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <array>
#include <map>
#include <algorithm>
#include <locale>

#include "config.hpp"
#include "esmtools.hpp"
#include "merger.hpp"

using namespace std;

class Converter
{
public:
	void convertEsm();
	void writeEsm();
	bool getConverterStatus() { return status; }

	Converter();
	Converter(string esm_path, Merger &m);

private:
	string intToByte(unsigned int x);
	bool caseInsensitiveStringCmp(string lhs, string rhs);
	void convertRecordContent(size_t pos, size_t old_size,
				  string new_text, size_t new_size);
	void convertScriptLine(int i, string id);
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

	bool status = {};
	Esmtools esm;
	Merger dict;
	int counter;
	string rec_content;
	string esm_content;
	string script_text;
};

#endif
