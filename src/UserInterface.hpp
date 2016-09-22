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
	void makeDictNot();
	void mergeDict();
	void convertEsm();
	void makeScriptText();
	void makeDiff();

	UserInterface(std::vector<std::string> &a);

private:
	void prepareUi(std::vector<std::string> &a);

	std::vector<std::string> arg;
	std::vector<std::string> path_esm;
	std::vector<std::string> path_dict_n;
	std::vector<std::string> path_dict_f;

};

#endif
