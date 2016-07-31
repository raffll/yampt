#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include "config.hpp"
#include "esmtools.hpp"
#include "merger.hpp"

class Converter
{
public:
	void convertEsm();
	void writeEsm();
	bool getConverterStatus() { return status; }

	Converter();
	Converter(std::string esm_path, Merger &m);

private:
	std::string intToByte(unsigned int x);
	bool caseInsensitiveStringCmp(std::string lhs, std::string rhs);
	void convertRecordContent(size_t pos, size_t old_size,
				  std::string new_text, size_t new_size);
	void convertScriptLine(int i, std::string id);
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
	std::string rec_content;
	std::string esm_content;
	std::string script_text;
};

#endif
