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
	bool getDictStatus() { return status; }
	string getDictName() { return name; }
	string getDictPrefix() { return prefix; }
	string getDictLog() { return log; }
	map<string, string> const& getDict() const { return dict; }

	dicttools() {}

private:
	void setDictStatus(int i);
	void setDictName(string path);
	void parseDict();
	bool validateRecLength(const string &pri, const string &sec);

	bool status = {};
	string name;
	string prefix;
	string content;
	string log;
	map<string, string> dict;
};

#endif
