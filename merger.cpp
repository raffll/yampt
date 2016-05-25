#include "merger.hpp"

//----------------------------------------------------------
merger::merger(const char* path_pri)
{
	printStatus(0);
	dict_unique[0].readDict(path_pri);
	dict_unique[0].printLog();
}

//----------------------------------------------------------
merger::merger(const char* path_pri, const char* path_sec)
{
	printStatus(0);
	dict_unique[0].readDict(path_pri);
	dict_unique[0].printLog();
	printStatus(1);
	dict_unique[1].readDict(path_sec);
	dict_unique[1].printLog();
}

//----------------------------------------------------------
void merger::printStatus(int i)
{
	if(quiet == 0)
	{
		cerr << "--> Loading dict " << i << ":" << endl;
	}
}

//----------------------------------------------------------
void merger::mergeDict() /*TODO*/
{
	for(int i = 0; i < 10; i++)
	{
		dict_merged[i].insert(dict_unique[0].dict[i].begin(), dict_unique[0].dict[i].end());
		dict_merged[i].insert(dict_unique[1].dict[i].begin(), dict_unique[1].dict[i].end());
	}
}

//----------------------------------------------------------
void merger::writeMerged()
{
	writeDict(dict_merged);
}

//----------------------------------------------------------
void merger::writeDiffLog()
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
	if(quiet == 0)
	{
		cerr << "--> Writing " << file_name_pri << endl;
		cerr << "--> Writing " << file_name_sec << endl;
	}
}
