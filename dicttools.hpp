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
	bool getStatus(int i) { return status[i]; }

	dicttools() {}

	array<dict_t, 10> dict_read;

private:
	enum st{not_loaded, loaded, missing_sep, too_long};
	void setStatus(int i, st e);

	void parseDict(int i);
	void validateRecLength(int i, const string &str, const size_t &size);

	array<string, 10> log;
	array<int, 10> status = {};
	array<string, 10> file_name;
	array<string, 10> file_content;
};

#endif
