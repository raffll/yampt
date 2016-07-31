#ifndef UI_HPP
#define UI_HPP

using namespace std;

#include "config.hpp"
#include "creator.hpp"
#include "merger.hpp"
#include "converter.hpp"

class Ui
{
public:
	void makeDictRaw();
	void makeDictBase();
	void makeDictAllRecords();
	void makeDictNotConverted();
	void mergeDictionaries();
	void convertFile();
	void writeScriptLog();
	void writeBinaryLog();
	void writeDifferencesLog();

	Ui(vector<string> &a);

private:
	vector<string> arg;

};

#endif
