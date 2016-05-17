#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <string>
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
	static bool quiet;

protected:
	void cutFileName(string &str);
	void cutNullChar(string &str);
	unsigned int byteToInt(const string &str);
};

#endif
