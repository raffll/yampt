#include "Config.hpp"

using namespace std;

//----------------------------------------------------------
vector<string> Config::key_message = {"messagebox", "say ", "say,", "choice"};
vector<string> Config::key_dial = {"addtopic"};
vector<string> Config::key_cell = {"positioncell", "getpccell", "aifollowcell",
				   "placeitemcell", "showmap", "aiescortcell"};

bool Config::allow_more_info = 0;
bool Config::replace_broken_chars = 0;
string Config::log;

//----------------------------------------------------------
Config::Config()
{

}

//----------------------------------------------------------
void Config::writeDict(const array<map<string, string>, 11> &dict, string name)
{
	if(getSize(dict) > 0)
	{
		ofstream file(name, ios::binary);
		for(size_t i = 0; i < dict.size(); ++i)
		{
			for(const auto &elem : dict[i])
			{
				file << sep[4]
				     << sep[1] << elem.first
				     << sep[2] << elem.second
				     << sep[3] << "\r\n";
			}
		}
		cout << "--> Writing " << to_string(getSize(dict)) <<
			" records to " << name << "...\r\n";
	}
	else
	{
		cout << "--> No records to make dictionary!\r\n";
	}
}

//----------------------------------------------------------
void Config::writeText(const string &text, string name)
{
	if(!text.empty())
	{
		ofstream file(name, ios::binary);
		cout << "--> Writing " << name << "...\r\n";
		file << text;
	}
}

//----------------------------------------------------------
int Config::getSize(const array<map<string, string>, 11> &dict)
{
	int size = 0;
	for(auto const &elem : dict)
	{
		size += elem.size();
	}
	return size;
}
