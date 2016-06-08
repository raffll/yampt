#include "merger.hpp"

//----------------------------------------------------------
merger::merger(string path_first)
{
	dict[0].readDict(path_first);
	if(dict[0].getDictStatus() == 1)
	{
		status = 1;
		name = dict[0].getDictPrefix();
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
		name = dict[1].getDictPrefix() + "-" + dict[0].getDictPrefix();
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
		name = dict[2].getDictPrefix() + "-" + dict[1].getDictPrefix() + "-" + dict[0].getDictPrefix();
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
					log += dict[i].getDictName() + "\t" + elem.first + " --- " + elem.second + "\r\n";
					log += dict[i - 1].getDictName() + "\t" + search->first + " >>> " + search->second + "\r\n";
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
			cerr << "Duplicate records not merged: " << duplicate << endl;
			cerr << "Duplicate records with different text not merged: " << different << endl;
		}
	}
}

//----------------------------------------------------------
void merger::writeMerged()
{
	if(status == 1)
	{
		string suffix;
		if(dict[1].getDictStatus() == 0 && dict[2].getDictStatus() == 0)
		{
			suffix = ".sorted.dic";
		}
		else
		{
			suffix = ".merged.dic";
		}
		ofstream file;
		file.open(name + suffix, ios::binary);
		for(const auto &elem : merged)
		{
			file << sep[1] << elem.first << sep[2] << elem.second << sep[3] << endl;
		}
		cerr << "Writing " << merged.size() << " records to " << name << suffix << "..." << endl;
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
					diff_first += sep[1] + elem.first + sep[2] + elem.second + sep[3] + "\r\n";
					diff_second += sep[1] + search->first + sep[2] + search->second + sep[3] + "\r\n";
				}
			}
		}
		if(!diff_first.empty() && !diff_second.empty())
		{
			ofstream file_first;
			ofstream file_second;
			file_first.open(dict[0].getDictPrefix() + ".diff.dic");
			file_second.open(dict[1].getDictPrefix() + ".diff.dic");
			file_first << diff_first;
			file_second << diff_second;
			cerr << "Writing " << dict[0].getDictPrefix() << ".diff.dic" << "..." << endl;
			cerr << "Writing " << dict[1].getDictPrefix() << ".diff.dic" << "..." << endl;
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
		ofstream file;
		file.open("yampt.log");
		file << dict[0].getDictLog() + dict[1].getDictLog() + dict[2].getDictLog() + log;
		cerr << "Writing yampt.log..." << endl;
	}
}
