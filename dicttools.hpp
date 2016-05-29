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

class dicttools
{
public:
	void readDict(const char* path);
	bool getStatus(int i) { return status[i]; }
	dict_t const& getDict(int i) const { return dict[i]; }

	dicttools() {}

private:
	enum st{not_loaded, loaded, missing_sep, too_long};
	void setStatus(int i, st e);

	void parseDict(int i);
	void validateRecLength(int i, const string &str, const size_t &size);

	array<dict_t, 10> dict;
	array<string, 10> log;
	array<int, 10> status = {};
	array<string, 10> name;
	array<string, 10> content;
};

#endif
