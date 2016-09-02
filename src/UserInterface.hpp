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
	void convertEsmWithDIAL();

	UserInterface(vector<string> &a);

private:
	void prepareUi(vector<string> &a);

	vector<string> arg;
	vector<string> path_esm;
	vector<string> path_dict;
	vector<string> path_dict_rev;
};

#endif
