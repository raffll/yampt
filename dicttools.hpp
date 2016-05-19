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
	void printStatus(int i);
	void printDict(int i);
	void parseDict(int i);
	void validateDict(int i);
	void validateRecLength(int i);
	bool getStatus(int i) { return status[i]; }

	dicttools();

private:
	array<int, 10> status = {};
	array<string, 10> file_name;
	array<string, 10> file_content;

	array<multimap<string, pair<size_t, string>>, 10> dict;
};

#endif
