#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <array>
#include <map>

#include "tools.hpp"
#include "esmtools.hpp"
#include "merger.hpp"

using namespace std;

class converter
{
public:
	void convertEsm();
	void writeEsm();

	void convertCell();
	void convertGmst();
	void convertFnam();
	void convertDesc();
	void convertBook();
	void convertFact();

	converter();
	converter(string esm_path, merger &m);

private:
	string intToByte(unsigned int x);

	esmtools esm;
	merger dict;
};

#endif
