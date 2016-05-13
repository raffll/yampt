#ifndef DICTTOOLS_HPP
#define DICTTOOLS_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <string>
#include <map>

#include "tools.hpp"

using namespace std;

class dicttools : public tools
{
public:
	bool getStatus();
	void printStatus();
	void printDict();

	dicttools();
	dicttools(const char* path, int i);

private:
	string file_name;
	string file_content;
	bool is_loaded;
	size_t pri_size, sec_size;
	string pri_text, sec_text;
	multimap<string, string> dict;

	void parseDict();
};

#endif
