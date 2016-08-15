#include "DictTools.hpp"

using namespace std;

//----------------------------------------------------------
DictTools::DictTools()
{
	status = 0;
	invalid_record = 0;
}

//----------------------------------------------------------
DictTools::DictTools(const DictTools& that) : status(that.status),
					      name(that.name),
					      name_prefix(that.name_prefix),
					      invalid_record(that.invalid_record),
					      invalid_record_log(that.invalid_record_log),
					      dict(that.dict)
{

}

//----------------------------------------------------------
DictTools& DictTools::operator=(const DictTools& that)
{
	status = that.status;
	name = that.name;
	name_prefix = that.name_prefix;
	invalid_record = that.invalid_record;
	invalid_record_log = that.invalid_record_log;
	dict = that.dict;
	return *this;
}

//----------------------------------------------------------
DictTools::~DictTools()
{

}

//----------------------------------------------------------
void DictTools::readFile(string path)
{
	ifstream file(path, ios::binary);
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
		if(!content.empty() && parseDict(content) == 1)
		{
			status = 1;
			setName(path);
		}
	}
	printStatus(path);
}

//----------------------------------------------------------
void DictTools::printStatus(string path)
{
	if(status == 0)
	{
		Config::appendLog("--> Error while loading " + path + " (wrong path or missing separator)!\r\n");
	}
	else
	{
		Config::appendLog("--> Loading " + path + "...\r\n");
		Config::appendLog("    --> Records loaded: " + to_string(getSize()) + "\r\n");
		Config::appendLog("    --> Records invalid: " + to_string(invalid_record) + "\r\n");
		Config::appendLog(invalid_record_log, 1);
	}
}

//----------------------------------------------------------
int DictTools::getSize()
{
	int size = 0;
	for(auto const &elem : dict)
	{
		size += elem.size();
	}
	return size;
}

//----------------------------------------------------------
void DictTools::setName(string path)
{
	name = path.substr(path.find_last_of("\\/") + 1);
	name_prefix = name.substr(0, name.find_last_of("."));
}

//----------------------------------------------------------
bool DictTools::parseDict(string &content)
{
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
			return 1;
			break;
		}
		else if(pos_beg > pos_mid ||
			pos_beg > pos_end ||
			pos_mid > pos_end ||
			pos_end == string::npos)
		{
			return 0;
			break;
		}
		else
		{
			pri_text = content.substr(pos_beg + Config::sep[1].size(),
						  pos_mid - pos_beg - Config::sep[1].size());
			sec_text = content.substr(pos_mid + Config::sep[2].size(),
						  pos_end - pos_mid - Config::sep[2].size());
			insertRecord(pri_text, sec_text);
			pos_beg++;
			pos_mid++;
			pos_end++;
		}
	}
}

//----------------------------------------------------------
void DictTools::insertRecord(const string &pri_text, const string &sec_text)
{
	if(pri_text.size() > 4)
	{
		if(pri_text.substr(0, 4) == "CELL")
		{
			if(dict[0].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else if(pri_text.substr(0, 4) == "GMST")
		{
			if(dict[1].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else if(pri_text.substr(0, 4) == "FNAM")
		{
			if(sec_text.size() > 31)
			{
				invalid_record_log += Config::sep_line +
						      "Text too long, more than 31 bytes (has " +
						      to_string(sec_text.size()) + "\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
			else if(dict[2].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else if(pri_text.substr(0, 4) == "DESC")
		{
			if(dict[3].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else if(pri_text.substr(0, 4) == "TEXT")
		{
			if(dict[4].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else if(pri_text.substr(0, 4) == "RNAM")
		{
			if(dict[5].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else if(pri_text.substr(0, 4) == "INDX")
		{
			if(dict[6].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else if(pri_text.substr(0, 4) == "DIAL")
		{
			if(dict[7].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else if(pri_text.substr(0, 4) == "INFO")
		{
			if(sec_text.size() > 511)
			{
				invalid_record_log += Config::sep_line +
						      "Text too long, more than 512 bytes (has " +
						      to_string(sec_text.size()) + "\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
			else if(dict[8].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else if(pri_text.substr(0, 4) == "BNAM")
		{
			if(dict[9].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else if(pri_text.substr(0, 4) == "SCPT")
		{
			if(dict[10].insert({pri_text, sec_text}).second == 0)
			{
				invalid_record_log += Config::sep_line +
						      "Duplicate record\r\n" +
						      Config::sep[1] + pri_text +
						      Config::sep[2] + sec_text +
						      Config::sep[3] + "\r\n";
				invalid_record++;
			}
		}
		else
		{
			invalid_record_log += Config::sep_line +
					      "Invalid record" +
					      Config::sep[1] + pri_text +
					      Config::sep[2] + sec_text +
					      Config::sep[3] + "\r\n";
			invalid_record++;
		}
	}
	else
	{
		invalid_record_log += Config::sep_line +
				      "Invalid record" +
				      Config::sep[1] + pri_text +
				      Config::sep[2] + sec_text +
				      Config::sep[3] + "\r\n";
		invalid_record++;
	}
}
