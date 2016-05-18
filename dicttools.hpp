#ifndef DICTTOOLS_HPP
#define DICTTOOLS_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <regex>
#include <string>
#include <map>

#include "tools.hpp"

using namespace std;

class dicttools : public tools
{
public:
	void readFile(const char* path, int i);
	void printStatus();
	void printDict();
	void parseDict();
	void validateDict();
	bool getStatus() { return status; }

	dicttools();

private:
	int status;
	string file_name;
	string file_content;

	multimap<string, pair<size_t, string>> dict_in;
};

#endif
