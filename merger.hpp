#ifndef MERGER_HPP
#define MERGER_HPP

#include "config.hpp"
#include "dicttools.hpp"

class Merger
{
public:
	void mergeDict();
	void writeMerged();
	void writeDiff();
	void writeLog();

	bool getMergerStatus() { return status; }
	std::map<std::string, std::string> const& getDict() const { return merged; }

	Merger() {}
	Merger(std::vector<std::string> &path);

private:
	std::vector<Dicttools> dict_coll;
	bool status = {};
	std::map<std::string, std::string> merged;
	std::string log;
};

#endif
