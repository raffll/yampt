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

enum RecType { CELL, DIAL, INDX, RNAM, DESC, GMST, FNAM, INFO, BNAM, SCTX, TEXT };
const vector<string> sep = {"^", "<h3>", "</h3>", "<hr>",
			    "<!-------------------------------------------------------------->\r\n",
			    "\r\n        "};

class Config
{
public:
	void writeDict(const array<map<string, string>, 11> &dict, string name);
	int getSize(const array<map<string, string>, 11> &dict);

	static vector<string> getKeyMessage() { return key_message; }
	static vector<string> getKeyDial() { return key_dial; }
	static vector<string> getKeyCell() { return key_cell; }

	static string getOutputSuffix() { return output_suffix; }
	static bool getAllowMoreInfo() { return allow_more_info; }

	static void appendLog(string message);
	void writeText(const string &text, string name);

	Config();

private:
	void readConfig();
	void printStatus();
	void parseConfig(string &content);
	void parseOutputSuffix(string &content);
	void parseAllowMoreThan512InfoString(string &content);

	static vector<string> key_message;
	static vector<string> key_dial;
	static vector<string> key_cell;

	static string output_suffix;
	static bool allow_more_info;
	static string log;

	bool status = 0;

};

#endif
