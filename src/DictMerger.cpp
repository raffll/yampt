#include "DictMerger.hpp"

using namespace std;

//----------------------------------------------------------
DictMerger::DictMerger()
{

}

//----------------------------------------------------------
DictMerger::DictMerger(vector<string> &path)
{
	for(auto &elem : path)
	{
		DictReader reader;
		reader.readFile(elem);
		dict_coll.push_back(reader);
		log += reader.getLog();
	}
	status = true;
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
						counter[0]++;
					}
					else if(search != dict[k].end() &&
						search->second != elem.second)
					{
						valid_ptr = &yampt::valid[0];
						makeLog(dict_coll[i].getName(), elem.first, elem.second, search->second);
						counter[1]++;
					}
					else
					{
						counter[2]++;
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
		}
		printLog();
	}
}

//----------------------------------------------------------
void DictMerger::findDiff()
{
	if(status == 1 && dict_coll.size() == 2)
	{
		for(size_t type = 0; type < 11; type++)
		{
			for(auto &elem : dict_coll[0].getDict()[type])
			{
				auto search = dict_coll[1].getDict()[type].find(elem.first);
				if(search != dict_coll[1].getDict()[type].end())
				{
					if(search->second != elem.second)
					{
						diff[0][type].insert({elem.first, elem.second});
						diff[1][type].insert({search->first, search->second});
					}
				}
			}
		}
	}
}

//----------------------------------------------------------
void DictMerger::makeLog(const string name, const string unique_key, const string friendly_old, const string friendly_new)
{
	log += *valid_ptr + " '" + unique_key + "' in '" + name + "'\r\n" +
	       "---" + "\r\n" +
	       friendly_old + "\r\n" +
	       "---" + "\r\n" +
	       friendly_new + "\r\n" +
	       "---" + "\r\n";
}

//----------------------------------------------------------
void DictMerger::printLog()
{
	cout << endl
	     << "    MERGED / REPLACED / IDENTICAL" << endl
	     << "    -----------------------------" << endl
	     << setw(10) << to_string(counter[0]) << " / "
	     << setw(8) << to_string(counter[1]) << " / "
	     << setw(9) << to_string(counter[2])
	     << endl << endl;
}
