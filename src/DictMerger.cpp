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
						counter_merged++;
					}
					else if(search != dict[k].end() &&
						search->second != elem.second)
					{
						valid_ptr = &yampt::valid[0];
						makeLog(dict_coll[i].getName(), elem.first, elem.second, search->second);
						counter_replaced++;
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
		}
		printLog();
	}
}

//----------------------------------------------------------
void DictMerger::makeLog(const string name, const string unique_key, const string friendly_old, const string friendly_new)
{
	log += "Dictionary: " + name + "\r\n" +
	       "Record    : " + unique_key + "\r\n" +
	       "Result    : " + *valid_ptr + "\r\n" +
	       "--------------------------------------------------" + "\r\n" +
	       friendly_old + "\r\n" +
	       "--------------------------------------------------" + "\r\n" +
	       friendly_new + "\r\n" +
	       "--------------------------------------------------" + "\r\n\r\n\r\n";
}

//----------------------------------------------------------
void DictMerger::printLog()
{
	cout << endl
	     << "    Merged / Replaced / Identical" << endl
	     << "    -----------------------------" << endl
	     << setw(10) << to_string(counter_merged) << " / "
	     << setw(8) << to_string(counter_replaced) << " / "
	     << setw(9) << to_string(counter_identical)
	     << endl << endl;
}
