#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
Dicttools::Dicttools(const Dicttools& that) : status(that.status), name(that.name),
					      prefix(that.prefix), content(that.content),
					      invalid_record(that.invalid_record),
					      dict(that.dict)
{

}

//----------------------------------------------------------
Dicttools& Dicttools::operator=(const Dicttools& that)
{
	status = that.status;
	name = that.name;
	prefix = that.prefix;
	content = that.content;
	invalid_record = that.invalid_record;
	dict = that.dict;
	return *this;
}

//----------------------------------------------------------
Dicttools::~Dicttools()
{

}

//----------------------------------------------------------
void Dicttools::readDict(string path)
{
	ifstream file(path, ios::binary);
	if(file)
	{
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
			setDictName(path);
			parseDict(path);
		}
		else
		{
			setDictStatus(0, path);
		}
	}
	else
	{
		setDictStatus(0, path);
	}
}

//----------------------------------------------------------
void Dicttools::setDictStatus(bool st, string path)
{
	if(st == 0)
	{
		Config::appendLog("--> Error while loading " + path + " (wrong path or missing separator)!\r\n");
		status = 0;
	}
	else
	{
		Config::appendLog("--> Loading " + path + "...\r\n");
		Config::appendLog("    --> Records loaded: " + to_string(dict.size()) + "\r\n");
		Config::appendLog("    --> Records invalid: " + to_string(invalid_record) + "\r\n");
		if(!invalid_record_log.empty())
		{
			invalid_record_log += "-----------------------------"
					      "-----------------------------\r\n";
			Config::appendLog(invalid_record_log, 1);
		}
		status = 1;
	}
}

//----------------------------------------------------------
void Dicttools::setDictName(string path)
{
	name = path.substr(path.find_last_of("\\/") + 1);
	prefix = name.substr(0, name.find_last_of("."));
}

//----------------------------------------------------------
void Dicttools::parseDict(string path)
{
	invalid_record = 0;
	size_t pos_beg = 0;
	size_t pos_mid = 0;
	size_t pos_end = 0;
	string pri_text;
	string sec_text;
	while(true)
	{
		pos_beg = content.find(Config::sep[1], pos_beg);
		pos_mid = content.find(Config::sep[2], pos_mid);
		pos_end = content.find(Config::sep[3], pos_end);
		if(pos_beg == string::npos &&
		   pos_mid == string::npos &&
		   pos_end == string::npos)
		{
			setDictStatus(1, path);
			break;
		}
		else if(pos_beg > pos_mid ||
			pos_beg > pos_end ||
			pos_mid > pos_end ||
			pos_end == string::npos)
		{
			setDictStatus(0, path);
			break;
		}
		else
		{
			pri_text = content.substr(pos_beg + Config::sep[1].size(),
						  pos_mid - pos_beg - Config::sep[1].size());
			sec_text = content.substr(pos_mid + Config::sep[2].size(),
						  pos_end - pos_mid - Config::sep[2].size());
			if(validateRecLength(pri_text, sec_text))
			{
				if(dict.insert({pri_text, sec_text}).second == 0)
				{
					invalid_record_log += "-----------------------------"
							      "-----------------------------\r\n"
							      "Duplicate record\r\n" +
							      Config::sep[1] + pri_text +
							      Config::sep[2] + sec_text +
							      Config::sep[3] + "\r\n";
					invalid_record++;
				}
			}
			pos_beg++;
			pos_mid++;
			pos_end++;
		}
	}
}

//----------------------------------------------------------
bool Dicttools::validateRecLength(const string &pri, const string &sec)
{
	if(pri.size() > 4)
	{
		if((pri.substr(0, 4) == "FNAM" || pri.substr(0, 4) == "FACT") && sec.size() > 31)
		{
			invalid_record_log += "-----------------------------"
					      "-----------------------------\r\n"
					      "Text too long, more than 31 bytes (has " +
					      to_string(sec.size()) + "\r\n" +
					      Config::sep[1] + pri +
					      Config::sep[2] + sec +
					      Config::sep[3] + "\r\n";
			invalid_record++;
			return 0;
		}
		else if(pri.substr(0, 4) == "INFO" && sec.size() > 512)
		{
			invalid_record_log += "-----------------------------"
					      "-----------------------------\r\n"
					      "Text too long, more than 512 bytes (has " +
					      to_string(sec.size()) + "\r\n" +
					      Config::sep[1] + pri +
					      Config::sep[2] + sec +
					      Config::sep[3] + "\r\n";
			invalid_record++;
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		invalid_record_log += "-----------------------------"
				      "-----------------------------\r\n";
				      "Invalid record" +
				      Config::sep[1] + pri +
				      Config::sep[2] + sec +
				      Config::sep[3] + "\r\n";
		invalid_record++;
		return 0;
	}
}
