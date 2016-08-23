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
		DictReader reader;
		reader.readFile(elem);
		dicttools.push_back(reader);
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
void DictMerger::mergeDict()
{
	if(status == 1)
	{
		int identical_record = 0;
		int duplicate_record = 0;
		for(size_t i = 0; i < dicttools.size(); ++i)
		{
			for(size_t k = 0; k < dicttools[i].getDict().size(); ++k)
			{
				for(auto &elem : dicttools[i].getDict()[k])
				{
					auto search = dict[k].find(elem.first);
					if(search == dict[k].end())
					{
						dict[k].insert({elem.first, elem.second});
					}
					else if(search != dict[k].end() &&
						search->second != elem.second)
					{
						duplicate_record++;
						Config::appendLog(sep[4] +
								  dicttools[i].getName() + " >>> " +
								  dicttools[i - 1].getName() + "\r\n" +
								  sep[1] + elem.first +
								  sep[2] + elem.second +
								  sep[3] + "\r\n" +
								  sep[1] + search->first +
								  sep[2] + search->second +
								  sep[3] + "\r\n");
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
			cout << "--> Sorting complete!\r\n";
		}
		else
		{
			cout << "--> Merging complete!\r\n";
			cout << "    --> Records merged: " << to_string(getSize()) << "\r\n";
			cout << "    --> Duplicate records not merged: " <<
				to_string(duplicate_record) << "\r\n";
			cout << "    --> Identical records not merged: " <<
				to_string(identical_record) << "\r\n";
		}
	}
}

//----------------------------------------------------------
void DictMerger::writeDict()
{
	if(status == 1)
	{
		string name = "Merged.dic";
		ofstream file(Config::output_path + name, ios::binary);
		for(size_t i = 0; i < dict.size(); ++i)
		{
			for(const auto &elem : dict[i])
			{
				file << sep[4]
				     << sep[1] << elem.first
				     << sep[2] << elem.second
				     << sep[3] << "\r\n";
			}
		}
		cout << "--> Writing " << to_string(getSize()) <<
			" records to " << Config::output_path << name << "...\r\n";
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
