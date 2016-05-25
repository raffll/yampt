#include "merger.hpp"

//----------------------------------------------------------
merger::merger(const char* path_pri)
{
	dict_pri.readDictAll(path_pri);
}

//----------------------------------------------------------
merger::merger(const char* path_pri, const char* path_sec)
{
	dict_pri.readDictAll(path_pri);
	dict_sec.readDictAll(path_sec);
}

//----------------------------------------------------------
void merger::mergeDict(int i)
{
	dict_merged[i].insert(dict_pri.dict[i].begin(), dict_pri.dict[i].end());
	dict_merged[i].insert(dict_sec.dict[i].begin(), dict_sec.dict[i].end());
}

//----------------------------------------------------------
void merger::writeDict(int i)
{
	ofstream file;
	string file_name = "dict_" + to_string(i) + "_" + dict_name[i] + ".dic";
	file.open(file_name.c_str());
	for(const auto &elem : dict_merged[i])
	{
		file << line_sep[0] << elem.first << line_sep[1] << elem.second << line_sep[2] << endl;
	}
}

//----------------------------------------------------------
void merger::writeDuplicatesAll()
{
	for(int i = 0; i < 10; i++)
	{
		writeDuplicates(i);
	}
}

//----------------------------------------------------------
void merger::writeDuplicates(int i)
{
    ofstream file_pri;
    ofstream file_sec;
	string file_name_pri = "duplicates_1_" + to_string(i) + "_" + dict_name[i] + ".dic";
    string file_name_sec = "duplicates_2_" + to_string(i) + "_" + dict_name[i] + ".dic";
	file_pri.open(file_name_pri.c_str());
	file_sec.open(file_name_sec.c_str());

	for(auto &elem : dict_pri.dict[i])
	{
		auto search = dict_sec.dict[i].find(elem.first);
		if(search != dict_sec.dict[i].end())
		{
			if(search->second != elem.second)
			{
				file_pri << line_sep[0] << elem.first << line_sep[1] << elem.second << line_sep[2] << endl;
				file_sec << line_sep[0] << search->first << line_sep[1] << search->second << line_sep[2] << endl;
			}
		}
	}
}
