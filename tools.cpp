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

//----------------------------------------------------------
void tools::printDict(dict_t &dict)
{
	for(const auto &elem : dict)
	{
		cout << line_sep[0] << elem.first << line_sep[1] << elem.second << line_sep[2] << endl;
	}
}

//----------------------------------------------------------
void tools::writeDict(array<dict_t, 10> &dict)
{
	for(int i = 0; i < 10; i++)
	{
		ofstream file;
		string file_name = "dict_" + to_string(i) + "_" + dict_name[i] + ".dic";
		if(!dict[i].empty())
		{
			file.open(file_name.c_str());
			for(const auto &elem : dict[i])
			{
				file << line_sep[0] << elem.first << line_sep[1] << elem.second << line_sep[2] << endl;
			}
		}
		if(quiet == 0)
		{
			cerr << "--> Writing " << file_name << endl;
		}
	}
}

