#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "Config.hpp"
#include "DictReader.hpp"

class DictMerger
{
public:
	void mergeDict();
	void makeDiff();

	bool getStatus() { return status; }
	std::string getLog() { return log; }
	std::string getDiff(size_t i) { return diff[i]; }
	std::string getNamePrefix(size_t i) { return dict_coll[i].getNamePrefix(); }
	yampt::dict_t const& getDict() const { return dict; }

	DictMerger();
	DictMerger(std::vector<std::string> &path);

private:
	bool status = 0;
	int counter = 0;
	int counter_identical = 0;
	int counter_duplicate = 0;
	std::string log;
	std::vector<DictReader> dict_coll;
	yampt::dict_t dict;
	std::array<std::string, 2> diff;
};

#endif
