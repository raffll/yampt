#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "Config.hpp"
#include "DictReader.hpp"

using namespace std;

class DictMerger
{
public:
	void mergeDict();
	void writeDict();
	void writeCompare();

	bool getStatus() { return status; }
	array<map<string, string>, 11> const& getDict() const { return dict; }

	DictMerger();
	DictMerger(vector<string> &path);

private:
	int getSize();

	bool status;
	vector<DictReader> dicttools;
	array<map<string, string>, 11> dict;
};

#endif
