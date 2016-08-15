#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "Config.hpp"
#include "DictTools.hpp"

using namespace std;

class DictMerger
{
public:
	void mergeDict();
	void writeDict();
	void writeCompare();

	bool getStatus() { return status; }
	map<string, pair<string, size_t>> const& getDict(int i) const { return dict[i]; }

	DictMerger();
	DictMerger(vector<string> &path);

private:
	int getSize();

	bool status;
	vector<DictTools> dicttools;
	array<map<string, pair<string, size_t>>, 11> dict;
};

#endif
