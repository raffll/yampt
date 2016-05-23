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
	void readDictAll(const char* path);
	void readDict(const char* path, int i);
	void printDict(int i);
	size_t getBegin(int i);
	size_t getEnd(int i);
	bool getStatus(int i) { return status[i]; }

	dicttools() {}

	dict_t dict;

private:
	void printStatus(int i);
	void parseDict(int i);
	void validateDict(int i);
	void validateRecLength(int i);

	array<int, 10> status = {};
	array<string, 10> file_name;
	array<string, 10> file_content;
};

#endif
