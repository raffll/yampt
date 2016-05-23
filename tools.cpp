#include "tools.hpp"

using namespace std;

//----------------------------------------------------------
vector<string> tools::dict_name = {"cell", "gmst", "fnam", "desc", "book", "fact", "indx", "dial", "info", "scpt"};
vector<string> tools::key = {"Choice", "choice", "MessageBox", "Say ", "Say,", "say ", "say,"};
vector<string> tools::line_sep = {"<h3>", "</h3>", "<hr>"};
string tools::inner_sep = {"^"};
map<int, string> tools::type_coll = {{0, "T"}, {1, "V"}, {2, "G"}, {3, "P"}, {4, "J"}};
bool tools::quiet = 0;

//----------------------------------------------------------
void tools::cutFileName(string &str)
{
	string slash = "\\/";
	str = str.substr(str.find_last_of(slash) + 1);
}

//----------------------------------------------------------
void tools::cutNullChar(string &str)
{
	size_t is_null = str.find('\0');
	while(is_null != string::npos)
	{
		str.erase(is_null, 1);
		is_null = str.find('\0');
	}
}

//----------------------------------------------------------
unsigned int tools::byteToInt(const string &str)
{
	char buffer[4];
	unsigned char ubuffer[4];
	unsigned int number;

	str.copy(buffer, 4);

	for(int i = 0; i < 4; i++)
	{
		ubuffer[i] = buffer[i];
	}

	if(str.size() == 4)
	{
		return number = (ubuffer[0] | ubuffer[1] << 8 | ubuffer[2] << 16 | ubuffer[3] << 24);
	}
	else if(str.size() == 1)
	{
		return number = ubuffer[0];
	}
	else
	{
		return number = 0;
	}
}
