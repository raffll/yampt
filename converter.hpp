#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <array>
#include <map>

#include "creator.hpp"
#include "merger.hpp"

using namespace std;

class converter
{
public:
	string intToByte(unsigned int x);
	void printBinary(string str);
	void convertCell();
	void writeFile();

	converter();
	converter(const char* base_path, const char* dict_path);

private:
	esmtools base;
	dicttools dict;

	string file_name;
	string file_suffix;
	string current_rec;
	string file_content;
};

#endif
