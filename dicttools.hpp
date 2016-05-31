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

class dicttools
{
public:
	void readDict(string path);
	bool getDictStatus() { return dict_status; }
	string getDictName() { return dict_name; }
	string getDictLog() { return dict_log; }
	dict_t const& getDict() const { return dict; }

	dicttools() {}

private:
	void setDictStatus(int i);
	void setDictName(string path);
	void parseDict();
	bool validateRecLength(const string &pri, const string &sec);

	int dict_status = {};
	string dict_name;
	string dict_content;
	string dict_log;
	dict_t dict;
};

#endif
