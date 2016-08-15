#ifndef UI_HPP
#define UI_HPP

#include "Config.hpp"
#include "DictCreator.hpp"
#include "DictMerger.hpp"
#include "EsmConverter.hpp"

using namespace std;

class UserInterface
{
public:
	void makeDictRaw();
	void makeDictBase();
	void makeDictAll();
	void makeDictNot();
	void mergeDict();
	void convertEsm();
	void writeScripts();
	void writeCompare();

	UserInterface(vector<string> &a);

private:
	void prepareUi(vector<string> &a);

	vector<string> arg;
	vector<string> arg_file;
	vector<string> arg_dict;
	vector<string> arg_dict_rev;
};

#endif
