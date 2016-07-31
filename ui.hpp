#ifndef UI_HPP
#define UI_HPP

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

	Ui(std::vector<std::string> &a);

private:
	std::vector<std::string> arg;
};

#endif
