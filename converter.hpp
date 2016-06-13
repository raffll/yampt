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
};

#endif
