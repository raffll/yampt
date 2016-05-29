#include "merger.hpp"

//----------------------------------------------------------
merger::merger(const char* p1)
{
	printStatus(p1);
	dict_unique[0].readDict(p1);
}

//----------------------------------------------------------
merger::merger(const char* p1, const char* p2)
{
	printStatus(p1);
	dict_unique[0].readDict(p1);
	printStatus(p2);
	dict_unique[1].readDict(p2);
}

//----------------------------------------------------------
merger::merger(const char* p1, const char* p2, const char* p3)
{
	printStatus(p1);
	dict_unique[0].readDict(p1);
	printStatus(p2);
	dict_unique[1].readDict(p2);
	printStatus(p3);
	dict_unique[2].readDict(p3);
}

//----------------------------------------------------------
void merger::printStatus(const char * path)
{
	cerr << "--> Loading dict from " << path << ":" << endl;
}

//----------------------------------------------------------
void merger::mergeDict()
{
	for(int k = 0; k < 3; k++)
	{
		for(int i = 0; i < 10; i++)
		{
			for(auto &elem : dict_unique[k].dict[i])
			{
				auto search = dict_merged[i].find(elem.first);
				if(search == dict_merged[i].end())
				{
					dict_merged[i].insert({elem.first, elem.second});
				}
				if(search != dict_merged[i].end())
				{
					//
				}
			}
		}
	}
}

//----------------------------------------------------------
void merger::writeMerged()
{
	for(int i = 0; i < 10; i++)
	{
		ofstream file;
		string file_name = "dict_" + to_string(i) + "_" + dict_name[i] + ".dic";
		if(!dict_merged[i].empty())
		{
			file.open(file_name.c_str());
			for(const auto &elem : dict_merged[i])
			{
				file << line_sep[0] << elem.first << line_sep[1] << elem.second << line_sep[2] << endl;
			}
			cerr << "--> Writing " << file_name << endl;
		}
		else
		{
			cerr << "--> Skipping " << file_name << endl;
		}
	}
}

//----------------------------------------------------------
void merger::writeDiff()
{
    ofstream file_pri;
    ofstream file_sec;
	string file_name_pri = "diff_dict_0.dic";
	string file_name_sec = "diff_dict_1.dic";
	file_pri.open(file_name_pri.c_str());
	file_sec.open(file_name_sec.c_str());

	for(int i = 0; i < 10; i++)
	{
		for(auto &elem : dict_unique[0].dict[i])
		{
			auto search = dict_unique[1].dict[i].find(elem.first);
			if(search != dict_unique[1].dict[i].end())
			{
				if(search->second != elem.second)
				{
					file_pri << line_sep[0] << elem.first << line_sep[1] << elem.second << line_sep[2] << endl;
					file_sec << line_sep[0] << search->first << line_sep[1] << search->second << line_sep[2] << endl;
				}
			}
		}
	}
	cerr << "--> Writing " << file_name_pri << endl;
	cerr << "--> Writing " << file_name_sec << endl;
}
