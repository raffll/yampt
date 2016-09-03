#include "Config.hpp"

using namespace std;

//----------------------------------------------------------
vector<string> Config::key_message = {"messagebox", "say ", "say,", "choice"};
vector<string> Config::key_dial = {"addtopic"};
vector<string> Config::key_cell = {"positioncell", "getpccell", "aifollowcell",
				   "placeitemcell", "showmap", "aiescortcell"};

bool Config::allow_more_info = 0;
bool Config::replace_broken_chars = 0;
string Config::output_suffix;
string Config::log;

//----------------------------------------------------------
Config::Config()
{
	readConfig();
}

//----------------------------------------------------------
void Config::readConfig()
{
	ifstream file("yampt.cfg", ios::binary);
	if(file)
	{
		string content;
		char buffer[16384];
		size_t size = file.tellg();
		content.reserve(size);
		streamsize chars_read;
		while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			content.append(buffer, chars_read);
		}
		if(!content.empty())
		{
			status = 1;
			parseConfig(content);
		}
	}
	printStatus();
}

//----------------------------------------------------------
void Config::printStatus()
{
	if(status == 0)
	{
		cout << "--> Error while loading yampt.cfg!\r\n";
	}
	else
	{
		cout << "--> Loading yampt.cfg...\r\n";
	}
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

//----------------------------------------------------------
void Config::parseConfig(string &content)
{
	parseOutputSuffix(content);
	parseAllowMoreThan512InfoString(content);
	parseReplaceBrokenChars(content);
}

//----------------------------------------------------------
void Config::parseOutputSuffix(string &content)
{
	regex re("(OUTPUT_SUFFIX=)\"(.*?)\"");
	smatch found;
	regex_search(content, found, re);
	output_suffix = found[2].str();
}

//----------------------------------------------------------
void Config::parseAllowMoreThan512InfoString(string &content)
{
	regex re("(ALLOW_MORE_THAN_512_INFO_STRING=)(.)");
	smatch found;
	regex_search(content, found, re);
	if(found[2].str() == "1")
	{
		allow_more_info = 1;
	}
}

//----------------------------------------------------------
void Config::parseReplaceBrokenChars(string &content)
{
	regex re("(REPLACE_BROKEN_CHARS=)(.)");
	smatch found;
	regex_search(content, found, re);
	if(found[2].str() == "1")
	{
		replace_broken_chars = 1;
	}
}
