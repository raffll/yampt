#include "DictMerger.hpp"

using namespace std;

//----------------------------------------------------------
DictMerger::DictMerger()
{

}

//----------------------------------------------------------
DictMerger::DictMerger(vector<string> &path, bool allow_more_info)
{
	for(auto &elem : path)
	{
		DictReader reader(allow_more_info);
		reader.readFile(elem);
		if(reader.getStatus() == true)
		{
			dict_coll.push_back(reader);
			status = true;
			log += reader.getLog();
		}
		else
		{
			status = false;
			break;
		}
	}
}

//----------------------------------------------------------
void DictMerger::mergeDict()
{
	if(status == true)
	{
		for(size_t i = 0; i < dict_coll.size(); ++i)
		{
			for(size_t k = 0; k < dict_coll[i].getDict().size(); ++k)
			{
				for(auto &elem : dict_coll[i].getDict()[k])
				{
					auto search = dict[k].find(elem.first);
					if(search == dict[k].end())
					{
						dict[k].insert({elem.first, elem.second});
						counter++;
					}
					else if(search != dict[k].end() &&
						search->second != elem.second)
					{
						counter_duplicate++;
						log += sep[4] +
						       sep[1] + elem.first + sep[2] + elem.second +
						       sep[3] +
						       " <!-- record in " + dict_coll[i].getName() +
						       " replaced by -->\r\n" +
						       sep[1] + search->first + sep[2] + search->second +
						       sep[3] + "\r\n";
					}
					else
					{
						counter_identical++;
					}
				}
			}
		}
		if(dict_coll.size() == 1)
		{
			cout << "--> Sorting complete!\r\n";
		}
		else
		{
			cout << "--> Merging complete!\r\n";
			cout << "    --> Records merged: " << to_string(counter) << "\r\n";
			cout << "    --> Duplicate records not merged: " <<
				to_string(counter_duplicate) << "\r\n";
			cout << "    --> Identical records not merged: " <<
				to_string(counter_identical) << "\r\n";
		}
	}
}

//----------------------------------------------------------
void DictMerger::makeDiff()
{
	if(status == true && dict_coll.size() == 2)
	{
		for(size_t k = 0; k < 11; ++k)
		{
			for(auto &elem : dict_coll[0].getDict()[k])
			{
				auto search = dict_coll[1].getDict()[k].find(elem.first);
				if(search != dict_coll[1].getDict()[k].end())
				{
					if(search->second != elem.second)
					{
						diff[0] += sep[4] +
							   sep[1] + elem.first +
							   sep[2] + elem.second +
							   sep[3] + "\r\n";
						diff[1] += sep[4] +
							   sep[1] + search->first +
							   sep[2] + search->second +
							   sep[3] + "\r\n";
					}
				}
			}
		}
		if(diff[0].empty() && diff[1].empty())
		{
			cout << "--> No differences between dictionaries!\r\n";
		}
	}
}
