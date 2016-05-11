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
	string line;
	string file_name = path;
	file_name += "dict_" + to_string(i) + "_" + dict_name[i] + ".txt";
	ifstream file(file_name.c_str());

	if(file.good())
	{
		while(getline(file, line))
		{
			dict.push_back(line);
		}
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
	cout << "Dict is loaded: " << is_loaded << endl;
}

//----------------------------------------------------------
void dicttools::printDict()
{
	for(const auto &elem : dict)
	{
		cout << elem << endl;
	}
}
