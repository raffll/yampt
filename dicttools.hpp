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
	void readDict(string path);
	bool getDictStatus(int i) { return dict_status[i]; }
	dict_t const& getDict(int i) const { return dict[i]; }

	dicttools() {}

private:
	enum st{not_loaded, loaded, missing_sep, too_long};
	void setDictStatus(int i, st e);

	void parseDict(int i);
	void validateRecLength(int i, const string &str, const size_t &size);

	array<int, 10> dict_status = {};
	array<string, 10> dict_content;
	array<string, 10> dict_log;
	array<dict_t, 10> dict;
};

#endif
