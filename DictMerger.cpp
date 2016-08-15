#include "DictMerger.hpp"

using namespace std;

//----------------------------------------------------------
DictMerger::DictMerger()
{
	status = 0;
}

//----------------------------------------------------------
DictMerger::DictMerger(vector<string> &path)
{
	for(auto &elem : path)
	{
		DictTools tmp;
		tmp.readFile(elem);
		dicttools.push_back(tmp);
	}
	for(auto &elem : dicttools)
	{
		if(elem.getStatus() == 1)
		{
			status = 1;
		}
		else
		{
			status = 0;
			break;
		}
	}
}

//----------------------------------------------------------
int DictMerger::getSize()
{
	int size = 0;
	for(auto const &elem : dict)
	{
		size += elem.size();
	}
	return size;
}

//----------------------------------------------------------
void DictMerger::mergeDict()
{
	int identical_record = 0;
	int duplicate_record = 0;
	if(status == 1)
	{
		for(size_t i = 0; i < dicttools.size(); i++)
		{
			for(size_t k = 0; k < 11; k++)
			{
				for(auto &elem : dicttools[i].getDict(k))
				{
					auto search = dict[k].find(elem.first);
					if(search == dict[k].end())
					{
						dict[k].insert({elem.first, make_pair(elem.second, i)});
					}
					else if(search != dict[k].end() &&
						search->second.first != elem.second)
					{
						duplicate_record++;
						Config::appendLog(Config::sep_line, 1);
						Config::appendLog(dicttools[i].getName() + " >>> " +
								  dicttools[i - 1].getName() + "\r\n", 1);
						Config::appendLog(Config::sep[1] + elem.first +
								  Config::sep[2] + elem.second +
								  Config::sep[3] + "\r\n", 1);
						Config::appendLog(Config::sep[1] + search->first +
								  Config::sep[2] + search->second.first +
								  Config::sep[3] + "\r\n", 1);
					}
					else
					{
						identical_record++;
					}
				}
			}
		}
		if(dicttools.size() == 1)
		{
			Config::appendLog("--> Sorting complete!\r\n");
		}
		else
		{
			Config::appendLog("--> Merging complete!\r\n");
			Config::appendLog("    --> Records merged: " + to_string(getSize()) + "\r\n");
			Config::appendLog("    --> Duplicate records not merged: " +
					  to_string(duplicate_record) + "\r\n");
			Config::appendLog("    --> Identical records not merged: " +
					  to_string(identical_record) + "\r\n");
		}
	}
}

//----------------------------------------------------------
void DictMerger::writeDict()
{
	if(status == 1)
	{
		string name = "yampt-merged.dic";
		ofstream file(Config::output_path + name, ios::binary);
		for(size_t i = 0; i < dict.size(); i++)
		{
			for(const auto &elem : dict[i])
			{
				file << "<!-- " << dicttools[elem.second.second].getName() << " -->\r\n"
				     << Config::sep[1] << elem.first
				     << Config::sep[2] << elem.second.first
				     << Config::sep[3] << "\r\n";
			}
		}
		Config::appendLog("--> Writing " + to_string(getSize()) +
				  " records to " + Config::output_path + name + "...\r\n");
	}
}

//----------------------------------------------------------
void DictMerger::writeCompare()
{
	if(status == 1 && dicttools.size() == 2)
	{
		array<string, 2> diff;
		for(size_t k = 0; k < 11; k++)
		{
			for(auto &elem : dicttools[0].getDict(k))
			{
				auto search = dicttools[1].getDict(k).find(elem.first);
				if(search != dicttools[1].getDict(k).end())
				{
					if(search->second != elem.second)
					{
						diff[0] += Config::sep_line +
							   Config::sep[1] + elem.first +
							   Config::sep[2] + elem.second +
							   Config::sep[3] + "\r\n";
						diff[1] += Config::sep_line +
							   Config::sep[1] + search->first +
							   Config::sep[2] + search->second +
							   Config::sep[3] + "\r\n";
					}
				}
			}
		}
		if(!diff[0].empty() && !diff[1].empty())
		{
			string name;
			for(size_t i = 0; i < diff.size(); ++i)
			{
				name = "yampt-diff-" + to_string(i) + "-" +
				       dicttools[i].getNamePrefix() + ".log";
				ofstream file(Config::output_path + name, ios::binary);
				file << diff[i];
				Config::appendLog("--> Writing " + Config::output_path + name + "...\r\n");
			}
		}
		else
		{
			Config::appendLog("--> No differences between dictionaries!\r\n");
		}
	}
}
