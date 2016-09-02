#ifndef DICTREADER_HPP
#define DICTREADER_HPP

#include "Config.hpp"

using namespace std;

class DictReader
{
public:
	void readFile(string path);
	bool getStatus() { return status; }
	string getName() { return name; }
	string getNamePrefix() { return name_prefix; }
	string getLog() { return log; }
	array<map<string, string>, 11> const& getDict() const { return dict; }

	DictReader();
	DictReader(const DictReader& that);
	DictReader& operator=(const DictReader& that);
	~DictReader();

private:
	void printStatus(string path);
	void setName(string path);
	bool parseDict(string &content);
	void insertRecord(const string &pri_text, const string &sec_text);

	bool status = 0;
	string name;
	string name_prefix;
	int counter = 0;
	int counter_invalid = 0;
	string log;
	array<map<string, string>, 11> dict;
};

#endif
