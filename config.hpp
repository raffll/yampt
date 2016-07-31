#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

class config
{
public:
	static array<string, 4> sep;

	static string converted_path;
	static string converted_suffix;

	static vector<string> key_message;
	static vector<string> key_dial;
	static vector<string> key_cell;

	void readConfig();

	config() {}

private:
	void setConfigStatus(bool st);

	string content;
	bool status;

};

#endif
