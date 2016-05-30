#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

const static vector<string> dict_name = {"dict_0_cell.dic", "dict_1_gmst.dic", "dict_2_fnam.dic",
										 "dict_3_desc.dic", "dict_4_book.dic", "dict_5_fact.dic",
										 "dict_6_indx.dic", "dict_7_dial.dic", "dict_8_info.dic",
										 "dict_9_scpt.dic"};
const static vector<string> key = {"Choice", "choice", "MessageBox", "Say ", "Say,", "say ", "say,"};
const static vector<string> line_sep = {"<h3>", "</h3>", "<hr>"};
const static string inner_sep = {"^"};
const static string conv_suffix = {"_CONVERTED"};
static map<int, string> type_coll = {{0, "T"}, {1, "V"}, {2, "G"}, {3, "P"}, {4, "J"}};
typedef map<string, string> dict_t;

#endif
