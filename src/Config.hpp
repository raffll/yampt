#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <locale>
#include <regex>

namespace yampt
{

enum r_type { CELL, DIAL, INDX, RNAM, DESC, GMST, FNAM, INFO, BNAM, SCTX, TEXT };

typedef std::array<std::map<std::string, std::string>, 11> dict_t;

const std::array<std::string, 5> dialog_type = {"T", "V", "G", "P", "J"};

const std::vector<std::string> sep = {"^", "<h3>", "</h3>", "<hr>"};
const std::string line = "----------------------------------------------------------";
const bool NEXT = true;

const std::vector<std::string> key_message = {"messagebox", "say ", "say,", "choice"};
const std::vector<std::string> key_dial = {"addtopic"};
const std::vector<std::string> key_cell = {"positioncell", "getpccell", "aifollowcell",
					   "placeitemcell", "showmap", "aiescortcell"};
const std::vector<std::string> result = {"Not converted", "Converted", "Skipped"};

}

class Writer
{
public:
	void writeDict(const yampt::dict_t &dict, std::string name);
	void writeText(const std::string &text, std::string name);
	int getSize(const yampt::dict_t &dict);
};

class Tools
{
protected:
	unsigned int convertByteArrayToInt(const std::string &str);
	std::string convertIntToByteArray(unsigned int x);
	bool caseInsensitiveStringCmp(std::string lhs, std::string rhs);
	void eraseNullChars(std::string &str);
	void replaceBrokenChars(std::string &str);
	std::string eraseCarriageReturnChar(std::string &str);
};

#endif
