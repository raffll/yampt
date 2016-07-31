#ifndef MERGER_HPP
#define MERGER_HPP

#include <array>
#include <map>

#include "config.hpp"
#include "dicttools.hpp"

using namespace std;

class merger
{
public:
	void mergeDict();
	void writeMerged();
	void writeDiff();
	void writeLog();

	bool getMergerStatus() { return status; }
	map<string, string> const& getDict() const { return merged; }

	merger() {}
	merger(string path_first);
	merger(string path_first, string path_second);
	merger(string path_first, string path_second, string path_third);

private:
	array<dicttools, 3> dict;
	bool status = {};
	map<string, string> merged;
	string log;
};

#endif
