#include "merger.hpp"

//----------------------------------------------------------
merger::merger(const char* path_1, const char* path_2)
{
	dict_1.readDictAll(path_1);
	dict_2.readDictAll(path_2);
}

//----------------------------------------------------------
void merger::mergeDict(int i)
{
	dict[i].insert(dict_1.dict[i].begin(), dict_1.dict[i].end());
	dict[i].insert(dict_2.dict[i].begin(), dict_2.dict[i].end());
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
    ofstream file_1;
    ofstream file_2;
	string file_name_1 = "file_1_" + to_string(i) + "_" + dict_name[i] + ".log";
    string file_name_2 = "file_2_" + to_string(i) + "_" + dict_name[i] + ".log";
	file_1.open(file_name_1.c_str());
	file_2.open(file_name_2.c_str());

	for(auto &elem : dict_1.dict[i])
	{
		auto search = dict_2.dict[i].find(elem.first);
		if(search != dict_2.dict[i].end())
		{
			if(search->second.second != elem.second.second)
			{
				file_1 << line_sep[0] << elem.first << line_sep[1] << elem.second.second << line_sep[2] << endl;
				file_2 << line_sep[0] << search->first << line_sep[1] << search->second.second << line_sep[2] << endl;
			}
		}
	}
	cout << file_name_1 << " created succesfully!" << endl;
	cout << file_name_2 << " created succesfully!" << endl;
}
