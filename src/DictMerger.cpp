#include "DictMerger.hpp"

using namespace std;

//----------------------------------------------------------
DictMerger::DictMerger()
{

}

//----------------------------------------------------------
DictMerger::DictMerger(vector<string> &path, bool more_info)
{
	for(auto &elem : path)
	{
		DictReader reader(more_info);
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
						counter_skipped++;
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
		cout << endl
		     << "               merged / replaced / skipped" << endl
		     << "    --------------------------------------" << endl
		     << "    Records"
		     << setw(10) << to_string(counter_merged) << " / "
		     << setw(8) << to_string(counter_replaced) << " / "
		     << setw(7) << to_string(counter_skipped)
		     << endl << endl;
	}
}

//----------------------------------------------------------
void DictMerger::makeLog(const string name, const string unique_key, const string friendly_r, const string friendly_n)
{
	log += "File:              | " + name + "\r\n" +
	       "Record:            | " + unique_key + "\r\n" +
	       "Result:            | " + *valid_ptr +
	       "\r\n<!---->\r\n" +
	       friendly_r +
	       "\r\n<!---->\r\n" +
	       friendly_n + "\r\n" +
	       yampt::line + "\r\n";
}
