#ifndef DICTREADER_HPP
#define DICTREADER_HPP

#include "Config.hpp"

class DictReader
{
public:
	void readFile(std::string path);
	bool getStatus() { return status; }
	std::string getName() { return name; }
	std::string getNamePrefix() { return name_prefix; }
	std::string getLog() { return log; }
	dict_t const& getDict() const { return dict; }

	DictReader();
	DictReader(const DictReader& that);
	DictReader& operator=(const DictReader& that);
	~DictReader();

private:
	void printStatus(std::string path);
	void setName(std::string path);
	bool parseDict(std::string &content);
	void insertRecord(const std::string &pri_text, const std::string &sec_text);

	bool status = 0;
	std::string name;
	std::string name_prefix;
	int counter = 0;
	int counter_invalid = 0;
	std::string log;
	dict_t dict;
};

#endif
