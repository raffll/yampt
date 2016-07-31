#ifndef DICTTOOLS_HPP
#define DICTTOOLS_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <string>
#include <map>

#include "config.hpp"

using namespace std;

class Dicttools
{
public:
	void readDict(string path);
	bool getDictStatus() { return status; }
	string getDictName() { return name; }
	string getDictPrefix() { return prefix; }
	string getDictLog() { return log; }
	map<string, string> const& getDict() const { return dict; }

	Dicttools() {}
	Dicttools(const Dicttools& that);
	Dicttools& operator=(const Dicttools& that);
	~Dicttools();

private:
	void setDictStatus(bool st, string path);
	void setDictName(string path);
	void parseDict(string path);
	bool validateRecLength(const string &pri, const string &sec);

	bool status = {};
	string name;
	string prefix;
	string content;
	string log;
	int invalid;
	map<string, string> dict;
};

#endif
