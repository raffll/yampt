#ifndef MERGER_HPP
#define MERGER_HPP

#include <array>
#include <map>

#include "config.hpp"
#include "dicttools.hpp"

using namespace std;

class Merger
{
public:
	void mergeDict();
	void writeMerged();
	void writeDiff();
	void writeLog();

	bool getMergerStatus() { return status; }
	map<string, string> const& getDict() const { return merged; }

	Merger() {}
	Merger(vector<string> &path);

private:
	vector<Dicttools> dict_coll;
	bool status = {};
	map<string, string> merged;
	string log;
};

#endif
