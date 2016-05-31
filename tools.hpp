#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

const static vector<string> sep = {"^", "<h3>", "</h3>", "<hr>"};
const static vector<string> key = {"Choice", "choice", "MessageBox", "Say ", "Say,", "say ", "say,"};
static map<int, string> type_coll = {{0, "T"}, {1, "V"}, {2, "G"}, {3, "P"}, {4, "J"}};

#endif
