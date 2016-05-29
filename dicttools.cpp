#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
void dicttools::readDict(const char* path)
{
	for(int i = 0; i < 10; i++)
	{
		file_name[i] = path;
		file_name[i] += "dict_" + to_string(i) + "_" + dict_name[i] + ".dic";
		ifstream file(file_name[i].c_str());
		file_name[i] = file_name[i].substr(file_name[i].find_last_of("\\/") + 1);

		if(file)
		{
			char buffer[16384];
			size_t file_size = file.tellg();
			file_content[i].reserve(file_size);
			streamsize chars_read;

			while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
			{
				file_content[i].append(buffer, chars_read);
			}
			parseDict(i);
		}
		else
		{
			setStatus(i, not_loaded);
		}
	}
}

//----------------------------------------------------------
void dicttools::setStatus(int i, st e)
{
	switch(e)
	{
	case 0:
		cerr << file_name[i] << " status: Error while loading file!" << endl;
		status[i] = 0;
		break;

	case 1:
		cerr << file_name[i] << " status: OK" << endl;
		status[i] = 1;
		break;

	case 2:
		cerr << file_name[i] << " status: Missing separator!" << endl;
		status[i] = 0;
		dict_read[i].clear();
		break;

	case 3:
		cerr << file_name[i] << " status: Text too long!" << endl;
		cerr << log[i];
		status[i] = 0;
		dict_read[i].clear();
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
		pos_beg = file_content[i].find(line_sep[0], pos_beg);
		pos_mid = file_content[i].find(line_sep[1], pos_mid);
		pos_end = file_content[i].find(line_sep[2], pos_end);
		if(pos_beg == string::npos && pos_mid == string::npos && pos_end == string::npos)
		{
			if(log[i].empty())
			{
				setStatus(i, loaded);
			}
			else
			{
				setStatus(i, too_long);
			}
			break;
		}
		else if(pos_beg > pos_mid || pos_beg > pos_end || pos_mid > pos_end || pos_end == string::npos)
		{
			setStatus(i, missing_sep);
			break;
		}
		else
		{
			pri_text = file_content[i].substr(pos_beg + line_sep[0].size(), pos_mid - pos_beg - line_sep[0].size());
			sec_text = file_content[i].substr(pos_mid + line_sep[1].size(), pos_end - pos_mid - line_sep[1].size());
			dict_read[i].insert({pri_text, sec_text});
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
		log[2] += str + " <-- Text too long, more than 31 bytes! (has " + to_string(size) + ")\n";
	}
	if(i == 8 && size > 512)
	{
		log[8] += str + " <-- Text too long, more than 512 bytes! (has " + to_string(size) + ")\n";
	}
}
