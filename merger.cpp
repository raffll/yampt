#include "merger.hpp"

//----------------------------------------------------------
merger::merger(const char* path_1, const char* path_2)
{
	dict_1.readDictAll(path_1);
	dict_2.readDictAll(path_2);
}

//----------------------------------------------------------
merger::merger(const char* path_1, const char* path_2, const char* path_3)
{
	dict_1.readDictAll(path_1);
	dict_2.readDictAll(path_2);
	dict_3.readDictAll(path_3);
}

//----------------------------------------------------------
void merger::mergeDict(int i)
{
	dict[i].insert(dict_1.dict[i].begin(), dict_1.dict[i].end());
	dict[i].insert(dict_2.dict[i].begin(), dict_2.dict[i].end());
	dict[i].insert(dict_3.dict[i].begin(), dict_3.dict[i].end());
}

//----------------------------------------------------------
void merger::writeDict(int i)
{
	ofstream file;
	string file_name = "dict_" + to_string(i) + "_" + dict_name[i] + ".dic";
	file.open(file_name.c_str());
	for(const auto &elem : dict[i])
	{
		file << line_sep[0] << elem.first << line_sep[1] << elem.second.second << line_sep[2] << endl;
	}
}

//----------------------------------------------------------
void merger::findDuplicates(int i)
{
	for(auto &elem : dict_1.dict[i])
	{
		auto search = dict_2.dict[i].find(elem.first);
		if(search != dict_2.dict[i].end())
		{
			if(search->second.second != elem.second.second)
			{
				cout << elem.first << " --- " << elem.second.second << endl;
				cout << search->first << " >>> " << search->second.second << endl << endl;
			}
		}
	}
}

//----------------------------------------------------------
void merger::writeDuplicates(int i)
{
	ofstream file;
	string file_name = "dict_" + to_string(i) + "_" + dict_name[i] + ".log";
	file.open(file_name.c_str());

