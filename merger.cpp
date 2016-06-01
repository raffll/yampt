#include "merger.hpp"

//----------------------------------------------------------
merger::merger(string path_first)
{
	dict[0].readDict(path_first);
	if(dict[0].getDictStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
merger::merger(string path_first, string path_second)
{
	dict[0].readDict(path_first);
	dict[1].readDict(path_second);
	if(dict[0].getDictStatus() == 1 && dict[1].getDictStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
merger::merger(string path_first, string path_second, string path_third)
{
	dict[0].readDict(path_first);
	dict[1].readDict(path_second);
	dict[2].readDict(path_third);
	if(dict[0].getDictStatus() == 1 && dict[1].getDictStatus() == 1 && dict[2].getDictStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
void merger::mergeDict()
{
	int duplicate = 0;
	int different = 0;

	if(status == 1)
	{
		for(size_t i = 0; i < dict.size(); i++)
		{
			for(auto &elem : dict[i].getDict())
			{
				auto search = merged.find(elem.first);
				if(search == merged.end())
				{
					merged.insert({elem.first, elem.second});
				}
				else if(search != merged.end() && search->second != elem.second)
				{
					different++;
					log += dict[i].getDictName() + "\t" + elem.first + " --- " + elem.second + "\n";
					log += dict[i - 1].getDictName() + "\t" + search->first + " >>> " + search->second + "\n";
				}
				else
				{
					duplicate++;
				}
			}
			if(dict[i].getDictStatus() == 1)
			{
				cerr << "Records loaded: " << dict[i].getDict().size() << endl;
			}
		}
		if(dict[1].getDictStatus() == 0 && dict[2].getDictStatus() == 0)
		{
			cerr << "Sorting complete!" << endl;
		}
		else
		{
			cerr << "Merging complete!" << endl;
			cerr << "Records merged: " << merged.size() << endl;
			cerr << "Duplicate text: " << duplicate << endl;
			cerr << "Different text: " << different << endl;
		}
	}
}

//----------------------------------------------------------
void merger::writeMerged()
{
	if(status == 1)
	{
		ofstream file;
		file.open("Merged.dic");
		for(const auto &elem : merged)
		{
			file << sep[1] << elem.first << sep[2] << elem.second << sep[3] << endl;
		}
		cerr << "Writing Merged.dic..." << endl;
	}
}

//----------------------------------------------------------
void merger::writeDiff()
{
	if(status == 1)
	{
		string diff_first;
		string diff_second;

		for(auto &elem : dict[0].getDict())
		{
			auto search = dict[1].getDict().find(elem.first);
			if(search != dict[1].getDict().end())
			{
				if(search->second != elem.second)
				{
					diff_first += sep[1] + elem.first + sep[2] + elem.second + sep[3] + "\n";
					diff_second += sep[1] + search->first + sep[2] + search->second + sep[3] + "\n";
				}
			}
		}
		if(!diff_first.empty() && !diff_second.empty())
		{
			ofstream file_first;
			ofstream file_second;
			file_first.open("Diff_" + dict[0].getDictName());
			file_second.open("Diff_" + dict[1].getDictName());
			file_first << diff_first;
			file_second << diff_second;
			cerr << "Writing Diff_" << dict[0].getDictName() << "..." << endl;
			cerr << "Writing Diff_" << dict[1].getDictName() << "..." << endl;
		}
		else
		{
			cerr << "No differences between dictionaries!" << endl;
		}
	}
}

//----------------------------------------------------------
void merger::writeLog()
{
	if(status == 1)
	{
		if(!log.empty())
		{
			ofstream file;
			file.open("Dict.log");
			file << dict[0].getDictLog() + dict[1].getDictLog() + dict[2].getDictLog() + log;
			cerr << "Writing Dict.log..." << endl;
		}
	}
}
