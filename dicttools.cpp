#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
void dicttools::readDict(string path)
{
	for(int i = 0; i < 10; i++)
	{
		ifstream file(path + dict_name[i]);

		if(file)
		{
			char buffer[16384];
			size_t size = file.tellg();
			dict_content[i].reserve(size);
			streamsize chars_read;

			while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
			{
				dict_content[i].append(buffer, chars_read);
			}
			parseDict(i);
		}
		else
		{
			setDictStatus(i, not_loaded);
		}
	}
}

//----------------------------------------------------------
void dicttools::setDictStatus(int i, st e)
{
	switch(e)
	{
	case 0:
		cerr << "Loading " << dict_name[i] << " status: Error while loading file!" << endl;
		dict_status[i] = 0;
		break;

	case 1:
		cerr << "Loading " << dict_name[i] << " status: OK" << endl;
		dict_status[i] = 1;
		break;

	case 2:
		cerr << "Loading " << dict_name[i] << " status: Missing separator!" << endl;
		dict_status[i] = 0;
		dict[i].clear();
		break;

	case 3:
		cerr << "Loading " << dict_name[i] << " status: Text too long!" << endl;
		cerr << dict_log[i];
		dict_status[i] = 0;
		dict[i].clear();
		break;
	}
}

//----------------------------------------------------------
void dicttools::parseDict(int i)
{
	size_t pos_beg = 0;
	size_t pos_mid = 0;
	size_t pos_end = 0;
	string pri_text;
	string sec_text;

	while(true)
	{
		pos_beg = dict_content[i].find(line_sep[0], pos_beg);
		pos_mid = dict_content[i].find(line_sep[1], pos_mid);
		pos_end = dict_content[i].find(line_sep[2], pos_end);
		if(pos_beg == string::npos && pos_mid == string::npos && pos_end == string::npos)
		{
			if(dict_log[i].empty())
			{
				setDictStatus(i, loaded);
			}
			else
			{
				setDictStatus(i, too_long);
			}
			break;
		}
		else if(pos_beg > pos_mid || pos_beg > pos_end || pos_mid > pos_end || pos_end == string::npos)
		{
			setDictStatus(i, missing_sep);
			break;
		}
		else
		{
			pri_text = dict_content[i].substr(pos_beg + line_sep[0].size(), pos_mid - pos_beg - line_sep[0].size());
			sec_text = dict_content[i].substr(pos_mid + line_sep[1].size(), pos_end - pos_mid - line_sep[1].size());
			dict[i].insert({pri_text, sec_text});
			validateRecLength(i, pri_text, sec_text.size());

			pos_beg++;
			pos_mid++;
			pos_end++;
		}
	}
}

//----------------------------------------------------------
void dicttools::validateRecLength(int i, const string &str, const size_t &size)
{
	if(i == 2 && size > 31)
	{
		dict_log[2] += str + " <-- Text too long, more than 31 bytes! (has " + to_string(size) + ")\n";
	}
	if(i == 8 && size > 512)
	{
		dict_log[8] += str + " <-- Text too long, more than 512 bytes! (has " + to_string(size) + ")\n";
	}
}
