#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
Dicttools::Dicttools(const Dicttools& that) : status(that.status), name(that.name),
					      prefix(that.prefix), content(that.content),
					      log(that.log), invalid(that.invalid),
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
	log = that.log;
	invalid = that.invalid;
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
		cerr << "--> Error while loading " << path << " (wrong path or missing separator)!" << endl;
		status = 0;
	}
	else
	{
		cerr << "--> Loading " << path << "..." << endl;
		cerr << "    --> Records loaded: " << dict.size() << endl;
		cerr << "    --> Records invalid: " << invalid << endl;
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
	invalid = 0;
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
			pri_text = content.substr(pos_beg + Config::sep[1].size(), pos_mid - pos_beg - Config::sep[1].size());
			sec_text = content.substr(pos_mid + Config::sep[2].size(), pos_end - pos_mid - Config::sep[2].size());
			if(validateRecLength(pri_text, sec_text))
			{
				if(dict.insert({pri_text, sec_text}).second == 0)
				{
					log += name + "\t" + pri_text + " <-- Duplicate record\r\n";
					invalid++;
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
			log += name + "\t" + pri +
			       " <-- Text too long, more than 31 bytes (has " +
			       to_string(sec.size()) + ")\r\n";
			invalid++;
			return 0;
		}
		else if(pri.substr(0, 4) == "INFO" && sec.size() > 512)
		{
			log += name + "\t" + pri +
			       " <-- Text too long, more than 512 bytes (has " +
			       to_string(sec.size()) + ")\r\n";
			invalid++;
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		log += name + "\t" + pri + " <-- Invalid record\r\n";
		invalid++;
		return 0;
	}
}
