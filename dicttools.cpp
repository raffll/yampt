#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
void dicttools::readDict(const char* path)
{
	for(int i = 0; i < 10; i++)
	{
		file_path[i] = path;
		file_path[i] += "dict_" + to_string(i) + "_" + dict_name[i] + ".dic";
		ifstream file(file_path[i].c_str());
		cutFileName(file_path[i]);

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
			parseDict(i);
			printStatus(i);
		}
		else
		{
			status[i] = 0;
			printStatus(i);
		}
	}
}

//----------------------------------------------------------
void dicttools::printStatus(int i)
{
	if(quiet == 0)
	{
		if(status[i] == 1)
		{
			cerr << file_path[i] << " status: OK" << endl;
		}
		else if(status[i] == 0)
		{
			cerr << file_path[i] << " status: Error while loading file!" << endl;
		}
		else if(status[i] == -1)
		{
			cerr << file_path[i] << " status: Missing separator!" << endl;
		}
	}
}

//----------------------------------------------------------
void dicttools::printLog()
{
	if(!log.empty())
	{
		cerr << "Text length log: " << endl;
		cerr << log;
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
	}
}

//----------------------------------------------------------
void dicttools::validateRecLength(int i, const string &str, const size_t &size)
{
	if(quiet == 0)
	{
		if(i == 2 && status[2] == 1 && size > 31)
		{
			log += str + " <-- Text too long, more than 31 bytes! (has " + to_string(size) + ")" + "\n";
		}
		if(i == 8 && status[8] == 1 && size > 512)
		{
			log += str + " <-- Text too long, more than 512 bytes! (has " + to_string(size) + ")" + "\n";
		}
	}
}
