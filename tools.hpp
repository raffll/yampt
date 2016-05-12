#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <string>
#include <vector>
#include <map>

using namespace std;

static vector<string> dict_name = {"cell", "gmst", "fnam", "desc", "book", "fact", "indx", "dial", "info", "scpt"};
static vector<string> key = {"Choice", "choice", "MessageBox", "Say ", "Say,", "say ", "say,"};
static map<int, string> type_coll = {{0, "T"}, {1, "V"}, {2, "G"}, {3, "P"}, {4, "J"}};
static vector<string> line_sep = {"<h3>", "</h3>", "<hr>"};
static string inner_sep = {"^"};

class tools
{
public:
	void setFileName(string &file_name);
};

#endif
