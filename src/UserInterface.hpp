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
	void makeDict();
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

	std::map<std::string, bool> option = {{"-a", 0},
					      {"-r", 0},
					      {"--with-dial", 0},
					      {"--safe", 0},
					      {"--no-duplicates", 0}};
	Config config;
};

#endif
