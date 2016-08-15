#ifndef DICTTOOLS_HPP
#define DICTTOOLS_HPP

#include "Config.hpp"

using namespace std;

class DictTools
{
public:
	void readFile(string path);
	bool getStatus() { return status; }
	string getName() { return name; }
	string getNamePrefix() { return name_prefix; }
	map<string, string> const& getDict(int i) const { return dict[i]; }

	DictTools();
	DictTools(const DictTools& that);
	DictTools& operator=(const DictTools& that);
	~DictTools();

private:
	void printStatus(string path);
	int getSize();
	void setName(string path);
	bool parseDict(string &content);
	void insertRecord(const string &pri_text, const string &sec_text);

	bool status;
	string name;
	string name_prefix;
	int invalid_record;
	string invalid_record_log;
	array<map<string, string>, 11> dict;
};

#endif
