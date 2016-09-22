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

enum RecType { CELL, DIAL, INDX, RNAM, DESC, GMST, FNAM, INFO, BNAM, SCTX, TEXT };

typedef std::array<std::map<std::string, std::string>, 11> dict_t;

const std::vector<std::string> sep = {"^", "<h3>", "</h3>", "<hr>"};
const std::string line = "----------------------------------------------------------";
const std::string ext = "_EXT";

const std::vector<std::string> key_message = {"messagebox", "say ", "say,", "choice"};
const std::vector<std::string> key_dial = {"addtopic"};
const std::vector<std::string> key_cell = {"positioncell", "getpccell", "aifollowcell",
					   "placeitemcell", "showmap", "aiescortcell"};

}

class Writer
{
public:
	static void writeDict(const yampt::dict_t &dict, std::string name);
	static void writeText(const std::string &text, std::string name);
	static int getSize(const yampt::dict_t &dict);

};

class Config
{
public:
	static void setAllowMoreInfo(bool x);
	static void setReplaceBrokenChars(bool x);
	static void setSafeConvert(bool x);
	static void setAddDialToInfo(bool x);

	static bool getAllowMoreInfo() { return allow_more_info; }
	static bool getReplaceBrokenChars() { return replace_broken_chars; }
	static bool getSafeConvert() { return safe_convert; }
	static bool getAddDialToInfo() { return add_dial_to_info; }

private:
	static bool allow_more_info;
	static bool replace_broken_chars;
	static bool safe_convert;
	static bool add_dial_to_info;

};

#endif
