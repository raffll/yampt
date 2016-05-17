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
	bool validateDict(string &file_content);
	void parseDict(string &file_content);

	bool getStatus() { return is_loaded; }

	dicttools();

private:
	string file_name;
	bool is_loaded;
	int dict_number;
	multimap<string, pair<size_t, string>> dict_in;
};

#endif
