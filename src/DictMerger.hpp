#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "Config.hpp"
#include "DictReader.hpp"

using namespace std;

class DictMerger
{
public:
	void mergeDict();

	bool getStatus() { return status; }
	string getLog() { return log; }
	array<map<string, string>, 11> const& getDict() const { return dict; }

	DictMerger();
	DictMerger(vector<string> &path);

private:
	bool status = 0;
	int counter = 0;
	int counter_identical = 0;
	int counter_duplicate = 0;
	string log;
	vector<DictReader> dict_coll;
	array<map<string, string>, 11> dict;
};

#endif
