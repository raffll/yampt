#include "tools.hpp"

using namespace std;

//----------------------------------------------------------
vector<string> tools::dict_name = {"cell", "gmst", "fnam", "desc", "book", "fact", "indx", "dial", "info", "scpt"};
vector<string> tools::key = {"Choice", "choice", "MessageBox", "Say ", "Say,", "say ", "say,"};
vector<string> tools::line_sep = {"<h3>", "</h3>", "<hr>"};
string tools::inner_sep = {"^"};
map<int, string> tools::type_coll = {{0, "T"}, {1, "V"}, {2, "G"}, {3, "P"}, {4, "J"}};
