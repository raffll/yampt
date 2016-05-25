#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
void dicttools::readDictAll(const char* path)
{
	for(int i = 0; i < 10; i++)
	{
		readDict(path, i);
	}
}

//----------------------------------------------------------
void dicttools::readDict(const char* path, int i)
{
	file_name[i] = path;
	file_name[i] += "dict_" + to_string(i) + "_" + dict_name[i] + ".dic";
	ifstream file(file_name[i].c_str());
	//cutFileName(file_name[i]);

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
		validateDict(i);
		printStatus(i);
		parseDict(i);
	}
	else
	{
		status[i] = 0;
		printStatus(i);
	}
}

//----------------------------------------------------------
void dicttools::printStatus(int i)
{
	if(quiet == 0)
	{
		if(status[i] == 1)
		{
			cout << file_name[i] << " status: OK" << endl;
		}
		else if(status[i] == 0)
		{
			cerr << file_name[i] << " status: Error while loading file!" << endl;
		}
		else if(status[i] == -1)
		{
			cerr << file_name[i] << " status: Missing separator!" << endl;
		}
	}
}

//----------------------------------------------------------
void dicttools::validateDict(int i)
{
	size_t pos_beg = 0;
	size_t pos_mid = 0;
	size_t pos_end = 0;

	while(true)
	{
		pos_beg = file_content[i].find(line_sep[0], pos_beg);
		pos_mid = file_content[i].find(line_sep[1], pos_mid);
		pos_end = file_content[i].find(line_sep[2], pos_end);
		if(pos_beg == string::npos && pos_mid == string::npos && pos_end == string::npos)
		{
			status[i] = 1;
			break;
		}
		else if(pos_beg > pos_mid || pos_beg > pos_end || pos_mid > pos_end || pos_end == string::npos)
		{
			status[i] = -1;
			break;
		}
		else
		{
			pos_beg++;
			pos_mid++;
			pos_end++;
		}
	}
}

//----------------------------------------------------------
void dicttools::parseDict(int i)
{
	if(status[i] == 1)
	{
		regex re("(<h3>)(.*?)(</h3>)((.|\n)*?)(<hr>)");
		sregex_iterator next(file_content[i].begin(), file_content[i].end(), re);
		sregex_iterator end;
		smatch m;

		while(next != end)
		{
			m = *next;
			validateRecLength(i, m.str(2), m.str(4).size());
			dict[i].insert({m.str(2), m.str(4)});

			next++;
		}
		file_content[i].erase();
	}
}

//----------------------------------------------------------
void dicttools::validateRecLength(int i, const string &str, const size_t &size)
{
	if(i == 2 && status[2] == 1)
	{
		if(size > 31)
		{
			cerr << str << " <-- Text too long, more than 31 bytes! (has " << size << ")" << endl;
		}
	}
	if(i == 8 && status[8] == 1)
	{
		if(size > 511)
		{
			cerr << str << " <-- Text too long, more than 511 bytes! (has " << size << ")" << endl;
		}
	}
}
