#ifndef USERINTERFACE_HPP
#define USERINTERFACE_HPP

#include "Config.hpp"
#include "DictCreator.hpp"
#include "DictMerger.hpp"
#include "EsmConverter.hpp"

class UserInterface
{
public:
	void makeDictRaw();
	void makeDictBase();
	void makeDictAll();
	void makeDictNotFound();
	void makeDictChanged();
	void mergeDict();
	void convertEsm();

	void findDiff();
	void binaryDump();
	void wordList();

	UserInterface(std::vector<std::string> &a);

private:
	Writer writer;

	std::vector<std::string> arg;
	std::vector<std::string> file_p;
	std::vector<std::string> dict_p;
	std::vector<std::string> output;

	bool add_dial = false;
	bool safe = false;
};

#endif
