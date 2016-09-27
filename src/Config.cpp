#include "Config.hpp"

using namespace std;

//----------------------------------------------------------
void Writer::writeDict(const yampt::dict_t &dict, string name)
{
	if(getSize(dict) > 0)
	{
		ofstream file(name, ios::binary);
		for(size_t i = 0; i < dict.size(); ++i)
		{
			for(const auto &elem : dict[i])
			{
				file << yampt::line << "\r\n"
				     << yampt::sep[1] << elem.first
				     << yampt::sep[2] << elem.second
				     << yampt::sep[3] << "\r\n";
			}
		}
		cout << "--> Writing " << to_string(getSize(dict)) <<
			" records to " << name << "..." << endl << endl;
	}
	else
	{
		cout << "--> No records to make dictionary!" << endl << endl;
	}
}

//----------------------------------------------------------
void Writer::writeText(const string &text, string name, bool make_log)
{
	if(make_log == true)
	{
		ofstream file(name, ios::binary);
		file << text;
		cout << "--> Writing " << name << "..." << endl << endl;
	}
}

//----------------------------------------------------------
int Writer::getSize(const yampt::dict_t &dict)
{
	int size = 0;
	for(auto const &elem : dict)
	{
		size += elem.size();
	}
	return size;
}

//----------------------------------------------------------
unsigned int Tools::convertByteArrayToInt(const string &str)
{
	char buffer[4];
	unsigned char ubuffer[4];
	unsigned int x;
	str.copy(buffer, 4);
	for(int i = 0; i < 4; i++)
	{
		ubuffer[i] = buffer[i];
	}
	if(str.size() == 4)
	{
		return x = (ubuffer[0] | ubuffer[1] << 8 | ubuffer[2] << 16 | ubuffer[3] << 24);
	}
	else if(str.size() == 1)
	{
		return x = ubuffer[0];
	}
	else
	{
		return x = 0;
	}
}

//----------------------------------------------------------
string Tools::convertIntToByteArray(unsigned int x)
{
	char bytes[4];
	string str;
	copy(static_cast<const char*>(static_cast<const void*>(&x)),
	     static_cast<const char*>(static_cast<const void*>(&x)) + sizeof x,
	     bytes);
	for(int i = 0; i < 4; i++)
	{
		str.push_back(bytes[i]);
	}
	return str;
}

//----------------------------------------------------------
bool Tools::caseInsensitiveStringCmp(string lhs, string rhs)
{
	transform(lhs.begin(), lhs.end(), lhs.begin(), ::toupper);
	transform(rhs.begin(), rhs.end(), rhs.begin(), ::toupper);
	if(lhs == rhs)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//----------------------------------------------------------
void Tools::eraseNullChars(string &str)
{
	size_t is_null = str.find('\0');
	if(is_null != string::npos)
	{
		str.erase(is_null);
	}
}

//----------------------------------------------------------
void Tools::replaceBrokenChars(string &str)
{
	for(size_t i = 0; i < str.size(); i++)
	{
		if(static_cast<int>(str[i]) == -127)
		{
			str[i] = '?';
		}
	}
}

//----------------------------------------------------------
string Tools::eraseCarriageReturnChar(string &str)
{
	if(str.find('\r') != string::npos)
	{
		str.erase(str.size() - 1);
	}
	return str;
}
