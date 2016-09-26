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
	void makeDict();
	void mergeDict();
	void convertEsm();
	void makeScriptText();
	void makeDiff();

	UserInterface(std::vector<std::string> &a);

private:
	Writer writer;

	std::vector<std::string> arg;
	std::vector<std::string> file_p;
	std::vector<std::string> dict_p;

	bool more_info = false;
	bool replace_broken = false;
	bool add_dial = false;
	bool convert_safe = false;
	bool no_duplicates = false;
};

#endif
