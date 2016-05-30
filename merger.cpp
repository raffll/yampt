#include "merger.hpp"

//----------------------------------------------------------
merger::merger(vector<string>& path)
{
	for(size_t i = 0; i < path.size(); i++)
	{
		cerr << "--> Loading dict from " << path[i] << endl;
		dicttools temp;
		dict_tool.push_back(temp);
		dict_tool[i].readDict(path[i]);
	}
}

//----------------------------------------------------------
void merger::mergeDict()
{
	for(size_t k = 0; k < dict_tool.size(); k++)
	{
		for(int i = 0; i < 10; i++)
		{
			for(auto &elem : dict_tool[k].getDict(i))
			{
				auto search = dict[i].find(elem.first);
				if(search == dict[i].end())
				{
					dict[i].insert({elem.first, elem.second});
				}
			}
		}
	}
	cerr << "--> Merging complete!" << endl;
}

//----------------------------------------------------------
void merger::writeMerged()
{
	for(int i = 0; i < 10; i++)
	{
		ofstream file;
		if(!dict[i].empty())
		{
			file.open(dict_name[i]);
			for(const auto &elem : dict[i])
			{
				file << line_sep[0] << elem.first << line_sep[1] << elem.second << line_sep[2] << endl;
			}
			cerr << "--> Writing " << dict_name[i] << endl;
		}
		else
		{
			cerr << "--> Skipping " << dict_name[i] << endl;
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
	file_pri.open(file_name_pri);
	file_sec.open(file_name_sec);

	for(int i = 0; i < 10; i++)
	{
		for(auto &elem : dict_tool[0].getDict(i))
		{
			auto search = dict_tool[1].getDict(i).find(elem.first);
			if(search != dict_tool[1].getDict(i).end())
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
