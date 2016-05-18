#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
dicttools::dicttools()
{
	status = 0;
}

//----------------------------------------------------------
void dicttools::readFile(const char* path, int i)
{
	file_name = path;
	file_name += "dict_" + to_string(i) + "_" + dict_name[i] + ".dic";
	ifstream file(file_name.c_str());
	cutFileName(file_name);

	if(file)
	{
		char buffer[16384];
		size_t file_size = file.tellg();
		file_content.reserve(file_size);
		streamsize chars_read;

		while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			file_content.append(buffer, chars_read);
		}
		validateDict();
		printStatus();
	}
	else
	{
		status = 0;
		printStatus();
	}
}

//----------------------------------------------------------
void dicttools::printStatus()
{
	if(quiet == 0)
	{
		if(status == 1)
		{
			cout << file_name << " status: OK" << endl;
		}
		else if(status == 0)
		{
			cerr << file_name << " status: Error while loading file!" << endl;
		}
		else if(status == -1)
		{
			cerr << file_name << " status: Missing separator!" << endl;
		}
		else
		{
			cerr << file_name << " status: Text too long, more than 32/512 bytes!" << endl;
		}
	}
}

//----------------------------------------------------------
void dicttools::printDict()
{
	if(status == 1)
	{
		for(const auto &elem : dict_in)
		{
			cout << line_sep[0] << elem.first << line_sep[1] << elem.second.second << line_sep[2] << endl;
		}
	}
}

//----------------------------------------------------------
void dicttools::validateDict()
{
	size_t pos_beg = 0;
	size_t pos_mid = 0;
	size_t pos_end = 0;

	while(true)
	{
		pos_beg = file_content.find(line_sep[0], pos_beg);
		pos_mid = file_content.find(line_sep[1], pos_mid);
		pos_end = file_content.find(line_sep[2], pos_end);
		if(pos_beg == string::npos && pos_mid == string::npos && pos_end == string::npos)
		{
			status = 1;
			break;
		}
		else if(pos_beg > pos_mid || pos_beg > pos_end || pos_mid > pos_end || pos_end == string::npos)
		{
			status = -1;
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
void dicttools::parseDict()
{
	if(status == 1)
	{
		regex re("(<h3>)(.*?)(</h3>)((.|\n)*?)(<hr>)");
		sregex_iterator next(file_content.begin(), file_content.end(), re);
		sregex_iterator end;
		smatch m;

		while(next != end)
		{
			m = *next;
			dict_in.insert({m.str(2), make_pair(m.str(4).size(), m.str(4))});
			next++;
		}
		file_content.erase();
	}
}

