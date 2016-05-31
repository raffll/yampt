#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
void dicttools::readDict(string path)
{
	ifstream file(path);
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
			parseDict();
		}
		else
		{
			setDictStatus(0);
		}
	}
	else
	{
		setDictStatus(0);
	}
}

//----------------------------------------------------------
void dicttools::setDictStatus(int st)
{
	if(st == 0)
	{
		cerr << "Error while loading dictionary (wrong path or missing separator)!" << endl;
		status = 0;
	}
	else
	{
		cerr << "Loading " << name << "..." << endl;
		status = 1;
	}
}

//----------------------------------------------------------
void dicttools::setDictName(string path)
{
	name = path.substr(path.find_last_of("\\/") + 1);
}

//----------------------------------------------------------
void dicttools::parseDict()
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
		if(pos_beg == string::npos && pos_mid == string::npos && pos_end == string::npos)
		{
			setDictStatus(1);
			break;
		}
		else if(pos_beg > pos_mid || pos_beg > pos_end || pos_mid > pos_end || pos_end == string::npos)
		{
			setDictStatus(0);
			break;
		}
		else
		{
			pri_text = content.substr(pos_beg + sep[1].size(), pos_mid - pos_beg - sep[1].size());
			sec_text = content.substr(pos_mid + sep[2].size(), pos_end - pos_mid - sep[2].size());

			if(validateRecLength(pri_text, sec_text))
			{
				dict.insert({pri_text, sec_text});
			}

			pos_beg++;
			pos_mid++;
			pos_end++;
		}
	}
}

//----------------------------------------------------------
bool dicttools::validateRecLength(const string &pri, const string &sec)
{
	if(pri.size() > 4 && pri.substr(0, 4) == "FNAM" && sec.size() > 31)
	{
		log += name + " " + pri + ": Text too long, more than 31 bytes! (has " + to_string(sec.size()) + ")\n";
		return 0;
	}
	else if(pri.size() > 4 && pri.substr(0, 4) == "INFO" && sec.size() > 512)
	{
		log += name + " " + pri + ": Text too long, more than 512 bytes! (has " + to_string(sec.size()) + ")\n";
		return 0;
	}
	else
	{
		return 1;
	}
}
