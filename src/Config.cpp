#include "Config.hpp"

using namespace std;
using namespace yampt;

//----------------------------------------------------------
bool Config::allow_more_info = false;
bool Config::replace_broken_chars = false;
bool Config::safe_convert = false;
bool Config::add_dial_to_info = false;

//----------------------------------------------------------
void Config::setAllowMoreInfo(bool x)
{
	allow_more_info = x;
	cout << "--> Allow more than 512 characters: " << std::boolalpha << allow_more_info << endl;
}

//----------------------------------------------------------
void Config::setReplaceBrokenChars(bool x)
{
	replace_broken_chars = x;
	cout << "--> Replace broken characters: " << std::boolalpha << replace_broken_chars << endl;
}

//----------------------------------------------------------
void Config::setSafeConvert(bool x)
{
	safe_convert = x;
	cout << "--> Safe convert: " << std::boolalpha << safe_convert << endl;
}

//----------------------------------------------------------
void Config::setAddDialToInfo(bool x)
{
	add_dial_to_info = x;
	cout << "--> Add dialog topic names to INFO records: " << std::boolalpha << add_dial_to_info << endl;
}

//----------------------------------------------------------
void Writer::writeDict(const dict_t &dict, string name)
{
	if(getSize(dict) > 0)
	{
		ofstream file(name, ios::binary);
		for(size_t i = 0; i < dict.size(); ++i)
		{
			for(const auto &elem : dict[i])
			{
				file << line << "\r\n"
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
void Writer::writeText(const string &text, string name)
{
	if(!text.empty())
	{
		ofstream file(name, ios::binary);
		cout << "--> Writing " << name << "...\r\n";
		file << text;
	}
}

//----------------------------------------------------------
int Writer::getSize(const dict_t &dict)
{
	int size = 0;
	for(auto const &elem : dict)
	{
		size += elem.size();
	}
	return size;
}
