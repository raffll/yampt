#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "Config.hpp"
#include "DictReader.hpp"

class DictMerger
{
public:
	void mergeDict();

	bool getStatus() { return status; }
	std::string getLog() { return log; }
	std::string getNamePrefix(size_t i) { return dict_coll[i].getNamePrefix(); }
	yampt::dict_t const& getDict() const { return dict; }

	DictMerger();
	DictMerger(std::vector<std::string> &path, bool more_info);

private:
	void makeLog(const std::string name, const std::string unique_key, const std::string friendly_r, const std::string friendly_n);
	void printLog();

	bool status = false;

	int counter_merged = 0;
	int counter_identical = 0;
	int counter_replaced = 0;

	const std::string *valid_ptr;

	std::string log;
	std::vector<DictReader> dict_coll;
	yampt::dict_t dict;
};

#endif
