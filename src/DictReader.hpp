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
	array<map<string, string>, 11> const& getDict() const { return dict; }

	DictReader();
	DictReader(const DictReader& that);
	DictReader& operator=(const DictReader& that);
	~DictReader();

private:
	int getSize();
	void printStatus(string path);
	void setName(string path);
	bool parseDict(string &content);
	void insertRecord(const string &pri_text, const string &sec_text);
	void appendInvalidRecordLog(const string &pri_text, const string &sec_text, string message);

	bool status;
	string name;
	string name_prefix;
	int invalid_record;
	array<map<string, string>, 11> dict;
};

#endif
