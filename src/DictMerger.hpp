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
	std::array<std::map<std::string, std::string>, 11> const& getDict() const { return dict; }

	DictMerger();
	DictMerger(std::vector<std::string> &path);

private:
	bool status = 0;
	int counter = 0;
	int counter_identical = 0;
	int counter_duplicate = 0;
	std::string log;
	std::vector<DictReader> dict_coll;
	std::array<std::map<std::string, std::string>, 11> dict;
};

#endif
