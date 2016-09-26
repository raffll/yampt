#include "DictReader.hpp"

using namespace std;

//----------------------------------------------------------
DictReader::DictReader(bool more_info)
{
	this->more_info = more_info;
}

//----------------------------------------------------------
DictReader::DictReader(const DictReader& that) : status(that.status),
					         name(that.name),
					         name_prefix(that.name_prefix),
					         counter(that.counter),
					         counter_invalid(that.counter_invalid),
					         dict(that.dict),
					         more_info(that.more_info)
{

}

//----------------------------------------------------------
DictReader& DictReader::operator=(const DictReader& that)
{
	status = that.status;
	name = that.name;
	name_prefix = that.name_prefix;
	counter = that.counter;
	counter_invalid = that.counter_invalid;
	dict = that.dict;
	more_info = that.more_info;
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
		setName(path);
		if(!content.empty() && parseDict(content) == true)
		{
			status = true;
		}
	}
	printStatus(path);
}

//----------------------------------------------------------
void DictReader::printStatus(string path)
{
	if(status == false)
	{
		cout << "--> Error while loading " << path << " (wrong path or missing separator)!\r\n";
	}
	else
	{
		cout << "--> Loading " << path << "...\r\n";
		cout << "    --> Records loaded: " << to_string(counter) << "\r\n";
		cout << "    --> Records invalid: " << to_string(counter_invalid) << "\r\n";
	}
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
		pos_beg = content.find(yampt::sep[1], pos_beg);
		pos_mid = content.find(yampt::sep[2], pos_mid);
		pos_end = content.find(yampt::sep[3], pos_end);
		if(pos_beg == string::npos &&
		   pos_mid == string::npos &&
		   pos_end == string::npos)
		{
			return true;
			break;
		}
		else if(pos_beg > pos_mid ||
			pos_beg > pos_end ||
			pos_mid > pos_end ||
			pos_end == string::npos)
		{
			return false;
			break;
		}
		else
		{
			pri_text = content.substr(pos_beg + yampt::sep[1].size(),
						  pos_mid - pos_beg - yampt::sep[1].size());
			sec_text = content.substr(pos_mid + yampt::sep[2].size(),
						  pos_end - pos_mid - yampt::sep[2].size());
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
			dict[yampt::r_type::CELL].insert({pri_text, sec_text});
			counter++;
		}
		else if(pri_text.substr(0, 4) == "GMST")
		{
			dict[yampt::r_type::GMST].insert({pri_text, sec_text});
			counter++;
		}
		else if(pri_text.substr(0, 4) == "FNAM")
		{
			if(sec_text.size() > 32)
			{
					log += yampt::sep[4] +
					       yampt::sep[1] + pri_text + yampt::sep[2] + sec_text + yampt::sep[3] +
					       " <!-- " + name +
					       " - Text too long, more than 32 bytes (has " +
					       to_string(sec_text.size()) + ") -->\r\n";
					counter_invalid++;
			}
			else
			{
				dict[yampt::r_type::FNAM].insert({pri_text, sec_text});
				counter++;
			}
		}
		else if(pri_text.substr(0, 4) == "DESC")
		{
			dict[yampt::r_type::DESC].insert({pri_text, sec_text});
			counter++;
		}
		else if(pri_text.substr(0, 4) == "TEXT")
		{
			dict[yampt::r_type::TEXT].insert({pri_text, sec_text});
			counter++;
		}
		else if(pri_text.substr(0, 4) == "RNAM")
		{
			dict[yampt::r_type::RNAM].insert({pri_text, sec_text});
			counter++;
		}
		else if(pri_text.substr(0, 4) == "INDX")
		{
			dict[yampt::r_type::INDX].insert({pri_text, sec_text});
			counter++;
		}
		else if(pri_text.substr(0, 4) == "DIAL")
		{
			dict[yampt::r_type::DIAL].insert({pri_text, sec_text});
			counter++;
		}
		else if(pri_text.substr(0, 4) == "INFO")
		{
			if(more_info == false)
			{
				if(sec_text.size() > 512)
				{
					log += yampt::sep[4] +
					       yampt::sep[1] + pri_text + yampt::sep[2] + sec_text + yampt::sep[3] +
					       " <!-- " + name +
					       " - Text too long, more than 512 bytes (has " +
					       to_string(sec_text.size()) + ") -->\r\n";
					counter_invalid++;
				}
				else
				{
					dict[yampt::r_type::INFO].insert({pri_text, sec_text});
					counter++;
				}
			}
			else
			{
				dict[yampt::r_type::INFO].insert({pri_text, sec_text});
				counter++;
			}
		}
		else if(pri_text.substr(0, 4) == "BNAM")
		{
			dict[yampt::r_type::BNAM].insert({pri_text, sec_text});
			counter++;
		}
		else if(pri_text.substr(0, 4) == "SCTX")
		{
			dict[yampt::r_type::SCTX].insert({pri_text, sec_text});
			counter++;
		}
		else
		{
			log += yampt::sep[4] +
			       yampt::sep[1] + pri_text + yampt::sep[2] + sec_text + yampt::sep[3] +
			       " <!-- " + name + " - Invalid record -->\r\n";
			counter_invalid++;
		}
	}
	else
	{
		log += yampt::sep[4] +
		       yampt::sep[1] + pri_text + yampt::sep[2] + sec_text + yampt::sep[3] +
		       " <!-- " + name + " - Invalid record -->\r\n";
		counter_invalid++;
	}
}
