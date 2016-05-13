#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
dicttools::dicttools()
{
	is_loaded = 0;
}

//----------------------------------------------------------
dicttools::dicttools(const char* path, int i)
{
	file_name = path;
	file_name += "dict_" + to_string(i) + "_" + dict_name[i] + ".txt";
	ifstream file(file_name.c_str());
	setFileName(file_name);

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
		file.close();
		is_loaded = 1;
	}
	else
	{
		is_loaded = 0;
	}
}

//----------------------------------------------------------
bool dicttools::getStatus()
{
	return is_loaded;
}

//----------------------------------------------------------
void dicttools::printStatus()
{
	cout << file_name << " is loaded: " << is_loaded << " with size: " << file_content.size() << endl;
}

//----------------------------------------------------------
void dicttools::printDict()
{
	cout << file_content << endl;
}

//----------------------------------------------------------
void dicttools::parseDict()
{
	if(is_loaded == 1)
	{

	}
}

