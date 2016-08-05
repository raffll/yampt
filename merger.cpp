#include "merger.hpp"

using namespace std;

//----------------------------------------------------------
Merger::Merger(vector<string> &path)
{
	for(auto &elem : path)
	{
		Dicttools tmp;
		tmp.readDict(elem);
		dict_coll.push_back(tmp);
	}
	for(auto &elem : dict_coll)
	{
		if(elem.getDictStatus() == 1)
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
void Merger::mergeDict()
{
	int identical_record = 0;
	int duplicate_record = 0;
	if(status == 1)
	{
		for(size_t i = 0; i < dict_coll.size(); i++)
		{
			for(auto &elem : dict_coll[i].getDict())
			{
				auto search = merged.find(elem.first);
				if(search == merged.end())
				{
					merged.insert({elem.first, elem.second});
				}
				else if(search != merged.end() && search->second != elem.second)
				{
					duplicate_record++;
					Config::appendLog("-----------------------------"
					                  "-----------------------------\r\n", 1);
					Config::appendLog(dict_coll[i].getDictName() + " >>> " +
					                  dict_coll[i - 1].getDictName() + "\r\n", 1);
					Config::appendLog(Config::sep[1] + elem.first +
							  Config::sep[2] + elem.second +
							  Config::sep[3] + "\r\n", 1);
					Config::appendLog(Config::sep[1] + search->first +
							  Config::sep[2] + search->second +
							  Config::sep[3] + "\r\n", 1);
				}
				else
				{
					identical_record++;
				}
			}
		}
		if(dict_coll.size() == 1)
		{
			Config::appendLog("--> Sorting complete!\r\n");
		}
		else
		{
			Config::appendLog("--> Merging complete!\r\n");
			Config::appendLog("    --> Records merged: " + to_string(merged.size()) + "\r\n");
			Config::appendLog("    --> Duplicate records not merged: " +
					  to_string(duplicate_record) + "\r\n");
			Config::appendLog("    --> Identical records not merged: " +
					  to_string(identical_record) + "\r\n");
		}
	}
}

//----------------------------------------------------------
void Merger::writeMerged()
{
	if(status == 1)
	{
		string name = "yampt-merged.dic";
		ofstream file(Config::output_path + name, ios::binary);
		for(const auto &elem : merged)
		{
			file << "<!-------------------------------"
				"------------------------------->\r\n";
			file << Config::sep[1] << elem.first
			     << Config::sep[2] << elem.second
			     << Config::sep[3] << "\r\n";
		}
		Config::appendLog("--> Writing " + to_string(merged.size()) +
				  " records to " + Config::output_path + name + "...\r\n");
	}
}

//----------------------------------------------------------
void Merger::writeDiff()
{
	if(status == 1)
	{
		array<string, 2> diff;
		for(auto &elem : dict_coll[0].getDict())
		{
			auto search = dict_coll[1].getDict().find(elem.first);
			if(search != dict_coll[1].getDict().end())
			{
				if(search->second != elem.second)
				{
					diff[0] += "<!-------------------------------"
						   "------------------------------->\r\n";
					diff[0] += Config::sep[1] + elem.first + Config::sep[2] +
						   elem.second + Config::sep[3] + "\r\n";
					diff[1] += "<!-------------------------------"
						   "------------------------------->\r\n";
					diff[1] += Config::sep[1] + search->first + Config::sep[2] +
					           search->second + Config::sep[3] + "\r\n";
				}
			}
		}
		if(!diff[0].empty() && !diff[1].empty())
		{
			string name;
			for(size_t i = 0; i < diff.size(); ++i)
			{
				name = "yampt-diff-" + to_string(i) + "-" +
				       dict_coll[i].getDictPrefix() + ".log";
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
