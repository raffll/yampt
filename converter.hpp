#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <array>
#include <map>

#include "creator.hpp"
#include "merger.hpp"

using namespace std;

class converter: public creator, public merger
{
public:
	string intToByte(unsigned int x);
	void printBinary(string str);
	void convertCell();

	converter();
	converter(const char* file, const char* dict);

	string current_rec;
	string file_content;
private:

};

#endif
