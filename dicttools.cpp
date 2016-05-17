#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
dicttools::dicttools()
{
	is_loaded = 0;
}

//----------------------------------------------------------
void dicttools::readFile(const char* path, int i)
{
	dict_number = i;
	file_name += "dict_" + to_string(i) + "_" + dict_name[i] + ".txt";
	string file_content;
	ifstream file(file_name.c_str());
	cutFileName(file_name);

	if(file.good())
	{
		char buffer[16384];
		size_t file_size = file.tellg();
		file_content.reserve(file_size);
		streamsize chars_read;

		while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			file_content.append(buffer, chars_read);
		}
		is_loaded = 1;
		cout << file_name << " is loaded: " << is_loaded << endl;
	}
	else
	{
		is_loaded = 0;
	}
}

//----------------------------------------------------------
void dicttools::printStatus()
{
	if(quiet == 0)
	{
		if(is_loaded == 1)
		{
			cout << file_name << " is loaded: " << is_loaded << endl;
		}
		else
		{
			cout << file_name << " is damaged - missing separator!" << endl;
		}
	}
}

//----------------------------------------------------------
void dicttools::printDict()
{
	for(const auto &elem : dict_in)
	{
		cout << elem.first << " " << elem.second.first << " " << elem.second.second << endl;
	}
}
