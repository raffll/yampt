#ifndef UI_HPP
#define UI_HPP

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
	void convertEsmWithDIAL();
	void convertEsmSafe();
	void makeScriptText();
	void makeDiff();

	void makeStats();
	void convertEsmStats();

	UserInterface(std::vector<std::string> &a);

private:
	void prepareUi(std::vector<std::string> &a);

	std::vector<std::string> arg;
	std::vector<std::string> path_esm;
	std::vector<std::string> path_dict;
	std::vector<std::string> path_dict_rev;
};

#endif
