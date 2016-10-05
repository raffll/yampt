#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "Config.hpp"
#include "DictReader.hpp"

class DictMerger
{
public:
	void mergeDict();
	void findDiff();

	bool getStatus() { return status; }
	std::string getLog() { return log; }
	std::string getNamePrefix(size_t i) { return dict_coll[i].getNamePrefix(); }
	yampt::dict_t const& getDict() const { return dict; }
	yampt::dict_t const& getDiff(size_t i) const { return diff[i]; }

	DictMerger();
	DictMerger(std::vector<std::string> &path);

private:
	void makeLog(const std::string name, const std::string unique_key, const std::string friendly_old, const std::string friendly_new);
	void printLog();

	bool status = false;

	int counter_merged = 0;
	int counter_identical = 0;
	int counter_replaced = 0;

	const std::string *valid_ptr;

	std::string log;
	std::vector<DictReader> dict_coll;

	yampt::dict_t dict;
	std::array<yampt::dict_t, 2> diff;
};

#endif
