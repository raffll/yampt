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
	void readDict(const char* path);
	void printLog();
	bool getStatus(int i) { return status[i]; }

	dicttools() {}

	array<dict_t, 10> dict;

private:
	void printStatus(int i);
	void parseDict(int i);
	void validateDict(int i);
	void validateRecLength(int i, const string &str, const size_t &size);

	string log;
	array<int, 10> status = {};
	array<string, 10> file_path;
	array<string, 10> file_content;
};

#endif
