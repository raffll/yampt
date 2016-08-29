#include "DictReader.hpp"

using namespace std;

//----------------------------------------------------------
DictReader::DictReader()
{
	status = 0;
	invalid_record = 0;
}

//----------------------------------------------------------
DictReader::DictReader(const DictReader& that) : status(that.status),
					         name(that.name),
					         name_prefix(that.name_prefix),
					         invalid_record(that.invalid_record),
					         dict(that.dict)
{

}

//----------------------------------------------------------
DictReader& DictReader::operator=(const DictReader& that)
{
	status = that.status;
	name = that.name;
	name_prefix = that.name_prefix;
	invalid_record = that.invalid_record;
	dict = that.dict;
	return *this;
}

//----------------------------------------------------------
DictReader::~DictReader()
{

}

//----------------------------------------------------------
void DictReader::readFile(string path)
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
void DictReader::printStatus(string path)
{
	if(status == 0)
	{
		cout << "--> Error while loading " << path << " (wrong path or missing separator)!\r\n";
	}
	else
	{
		cout << "--> Loading " << path << "...\r\n";
		cout << "    --> Records loaded: " << to_string(getSize()) << "\r\n";
		cout << "    --> Records invalid: " << to_string(invalid_record) << "\r\n";
	}
}

//----------------------------------------------------------
int DictReader::getSize()
{
	int size = 0;
	for(auto const &elem : dict)
	{
		size += elem.size();
	}
	return size;
}

//----------------------------------------------------------
void DictReader::setName(string path)
{
	name = path.substr(path.find_last_of("\\/") + 1);
	name_prefix = name.substr(0, name.find_last_of("."));
}

//----------------------------------------------------------
bool DictReader::parseDict(string &content)
{
	size_t pos_beg = 0;
	size_t pos_mid = 0;
	size_t pos_end = 0;
	string pri_text;
	string sec_text;
	while(true)
	{
		pos_beg = content.find(sep[1], pos_beg);
		pos_mid = content.find(sep[2], pos_mid);
		pos_end = content.find(sep[3], pos_end);
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
			pri_text = content.substr(pos_beg + sep[1].size(),
						  pos_mid - pos_beg - sep[1].size());
			sec_text = content.substr(pos_mid + sep[2].size(),
						  pos_end - pos_mid - sep[2].size());
			insertRecord(pri_text, sec_text);
			pos_beg++;
			pos_mid++;
			pos_end++;
		}
	}
}

//----------------------------------------------------------
void DictReader::insertRecord(const string &pri_text, const string &sec_text)
{
	if(pri_text.size() > 4)
	{
		if(pri_text.substr(0, 4) == "CELL")
		{
			dict[RecType::CELL].insert({pri_text, sec_text});
		}
		else if(pri_text.substr(0, 4) == "GMST")
		{
			dict[RecType::GMST].insert({pri_text, sec_text});
		}
		else if(pri_text.substr(0, 4) == "FNAM")
		{
			if(sec_text.size() > 31)
			{
				appendInvalidRecordLog(pri_text, sec_text,
						       "Text too long, more than 31 bytes (has " +
						       to_string(sec_text.size()) + ")");
			}
			else
			{
				dict[RecType::FNAM].insert({pri_text, sec_text});
			}
		}
		else if(pri_text.substr(0, 4) == "DESC")
		{
			dict[RecType::DESC].insert({pri_text, sec_text});
		}
		else if(pri_text.substr(0, 4) == "TEXT")
		{
			dict[RecType::TEXT].insert({pri_text, sec_text});
		}
		else if(pri_text.substr(0, 4) == "RNAM")
		{
			dict[RecType::RNAM].insert({pri_text, sec_text});
		}
		else if(pri_text.substr(0, 4) == "INDX")
		{
			dict[RecType::INDX].insert({pri_text, sec_text});
		}
		else if(pri_text.substr(0, 4) == "DIAL")
		{
			dict[RecType::DIAL].insert({pri_text, sec_text});
		}
		else if(pri_text.substr(0, 4) == "INFO")
		{
			if(sec_text.size() > 511)
			{
				appendInvalidRecordLog(pri_text, sec_text,
						       "Text too long, more than 511 bytes (has " +
						       to_string(sec_text.size()) + ")");
			}
			else
			{
				dict[RecType::INFO].insert({pri_text, sec_text});
			}
		}
		else if(pri_text.substr(0, 4) == "BNAM")
		{
			dict[RecType::BNAM].insert({pri_text, sec_text});
		}
		else if(pri_text.substr(0, 4) == "SCTX")
		{
			dict[RecType::SCTX].insert({pri_text, sec_text});
		}
		else
		{
			appendInvalidRecordLog(pri_text, sec_text, "Invalid record");
		}
	}
	else
	{
		appendInvalidRecordLog(pri_text, sec_text, "Invalid record");
	}
}

//----------------------------------------------------------
void DictReader::appendInvalidRecordLog(const string &pri_text, const string &sec_text, string message)
{
	Config::appendLog(sep[4] + message + "\r\n" +
			  sep[1] + pri_text +
			  sep[2] + sec_text +
			  sep[3] + "\r\n");
	invalid_record++;
}

