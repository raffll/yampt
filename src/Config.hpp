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

enum RecType { CELL, GMST, FNAM, DESC, TEXT, RNAM, INDX, DIAL, INFO, BNAM, SCTX };
const vector<string> sep = {"^", "<h3>", "</h3>", "<hr>",
			    "<!-------------------------------------------------------------->\r\n",
			    "\r\n        "};

class Config
{
public:
	static vector<string> key_message;
	static vector<string> key_dial;
	static vector<string> key_cell;

	static string output_suffix;
	static bool allow_more_info;

	static void appendLog(string message);
	void writeLog();

	Config();

private:
	void readConfig();
	void printStatus();
	void parseConfig(string &content);
	void parseOutputSuffix(string &content);
	void parseAllowMoreThan512InfoString(string &content);

	bool status;
	static string log;
};

#endif
