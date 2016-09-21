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

enum RecType { CELL, DIAL, INDX, RNAM, DESC, GMST, FNAM, INFO, BNAM, SCTX, TEXT };
const std::vector<std::string> sep = {"^", "<h3>", "</h3>", "<hr>",
				      "<!-------------------------------------------------------------->\r\n",
				      "\r\n        "};

class Config
{
public:
	void writeDict(const std::array<std::map<std::string, std::string>, 11> &dict, std::string name);
	void writeText(const std::string &text, std::string name);
	int getSize(const std::array<std::map<std::string, std::string>, 11> &dict);

	static std::vector<std::string> getKeyMessage() { return key_message; }
	static std::vector<std::string> getKeyDial() { return key_dial; }
	static std::vector<std::string> getKeyCell() { return key_cell; }

	Config();

private:
	static std::vector<std::string> key_message;
	static std::vector<std::string> key_dial;
	static std::vector<std::string> key_cell;

	static std::string log;

};

#endif
