#ifndef UI_HPP
#define UI_HPP

#include "config.hpp"
#include "creator.hpp"
#include "merger.hpp"
#include "converter.hpp"

class Ui
{
public:
	void makeDictRaw(vector<string> &arg_file);
	void makeDictBase(vector<string> &arg_file);
	void makeDictAllRecords(vector<string> &arg_file, vector<string> &arg_dict_rev);
	void makeDictNotConverted(vector<string> &arg_file, vector<string> &arg_dict_rev);
	void mergeDictionaries(vector<string> &arg_dict_rev);
	void convertFile(vector<string> &arg_file, vector<string> &arg_dict_rev);
	void writeScriptLog(vector<string> &arg_file);
	void writeDifferencesLog(vector<string> &arg_dict);

	Ui() {}

};

#endif
