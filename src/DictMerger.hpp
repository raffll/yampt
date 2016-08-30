#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "Config.hpp"
#include "DictTools.hpp"
#include "DictReader.hpp"

using namespace std;

class DictMerger : public DictTools
{
public:
	void mergeDict();
	void writeCompare();
	void convertDialInText();

	bool getStatus() { return status; }
	array<map<string, string>, 11> const& getDict() const { return dict; }

	DictMerger();
	DictMerger(vector<string> &path);

private:
	bool status;
	vector<DictReader> dicttools;
	array<map<string, string>, 11> dict;
};

#endif
