#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

class tools
{
public:
	static vector<string> dict_name;
	static vector<string> key;
	static vector<string> line_sep;
	static string inner_sep;
	static map<int, string> type_coll;

	typedef map<string, string> dict_t;
};

#endif
