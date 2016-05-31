#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <array>
#include <map>

#include "esmtools.hpp"
#include "merger.hpp"

using namespace std;

class converter
{
public:
	void writeEsm();
	void convertEsm();

	void convertCell();
	void convertGmst();
	void convertFnam();
	void convertDesc();

	converter();
	converter(string esm_path, vector<string> dict_path);

private:
	void printConverterLog(int i);
	string intToByte(unsigned int x);

	esmtools esm_tool;
	merger dict_tool;

	int conv_counter;
};

#endif
