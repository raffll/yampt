#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <array>
#include <map>
#include <algorithm>
#include <regex>

#include "tools.hpp"
#include "esmtools.hpp"
#include "merger.hpp"

using namespace std;

class converter
{
public:
	void convertEsm();
	void writeEsm();

	converter();
	converter(string esm_path, merger &m);

private:
	string intToByte(unsigned int x);
	bool caseInsensitiveStringCmp(string lhs, string rhs);
	void convertRecordContent(size_t pos, size_t old_size,
				  string new_text, size_t new_size);
	void convertScriptLine(int i, string id);
	void convertCell();
	void convertGmst();
	void convertFnam();
	void convertDesc();
	void convertBook();
	void convertFact();
	void convertIndx();
	void convertDial();
	void convertInfo();
	void convertBnam();
	void convertScpt();

	bool status = {};
	esmtools esm;
	merger dict;
	int counter;
	string rec_content;
	string esm_content;
	string script_text;
};

#endif
