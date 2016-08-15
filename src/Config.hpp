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

using namespace std;

class Config
{
public:
	static vector<string> sep;
	static string sep_line;
	static string base_dictionary_path;
	static string output_path;
	static string output_suffix;
	static vector<string> key_message;
	static vector<string> key_dial;
	static vector<string> key_cell;

	void readConfig();
	static void appendLog(string message, bool standard_output = 0);
	static void writeLog();

	Config();

private:
	void setConfigStatus(bool st);
	void parseOutputPath();
	void parseOutputSuffix();

	string content;
	static bool status;
	static string log;
};

#endif
