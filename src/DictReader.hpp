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
	yampt::dict_t const& getDict() const { return dict; }

	DictReader();
	DictReader(const DictReader& that);
	DictReader& operator=(const DictReader& that);
	~DictReader();

private:
	void printStatus(std::string path);
	void setName(std::string path);
	bool parseDict(std::string &content);
	void validateRecord();
	void insertRecord(yampt::r_type type);
	void makeLog();
	void makeLogHeader();
	void printLog();

	bool status = false;

	std::string name;
	std::string name_prefix;

	std::string unique_key;
	std::string friendly;

	const std::string *merger_log_ptr;

	int counter_loaded = 0;
	int counter_invalid = 0;
	int counter_toolong = 0;
	int counter_doubled = 0;
	int counter_all = 0;

	std::string log;
	yampt::dict_t dict;
};

#endif
