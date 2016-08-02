#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <string>
#include <array>
#include <map>
#include <algorithm>
#include <locale>
#include <regex>

class Config
{
public:
	static std::array<std::string, 4> sep;

	static std::string output_path;
	static std::string output_suffix;

	static std::vector<std::string> key_message;
	static std::vector<std::string> key_dial;
	static std::vector<std::string> key_cell;

	void readConfig();

	Config() {}

private:
	void setConfigStatus(bool st);
	void parseOutputPath();
	void parseOutputSuffix();

	std::string content;
	bool status;
};

#endif
