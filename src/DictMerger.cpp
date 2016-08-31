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
			cout << "    --> Records merged: " << to_string(getSize(dict)) << "\r\n";
			cout << "    --> Duplicate records not merged: " <<
				to_string(duplicate_record) << "\r\n";
			cout << "    --> Identical records not merged: " <<
				to_string(identical_record) << "\r\n";
		}
	}
}

//----------------------------------------------------------
void DictMerger::writeCompare()
{
	if(status == 1 && dicttools.size() == 2)
	{
		array<string, 2> diff;
		for(size_t k = 0; k < 11; ++k)
		{
			for(auto &elem : dicttools[0].getDict()[k])
			{
				auto search = dicttools[1].getDict()[k].find(elem.first);
				if(search != dicttools[1].getDict()[k].end())
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
		if(!diff[0].empty() && !diff[1].empty())
		{
			string name;
			for(size_t i = 0; i < diff.size(); ++i)
			{
				name = dicttools[i].getNamePrefix() + ".diff." + to_string(i) + ".log";
				ofstream file(name, ios::binary);
				file << diff[i];
				cout << "--> Writing " + name + "...\r\n";
			}
		}
		else
		{
			cout << "--> No differences between dictionaries!\r\n";
		}
	}
}
//----------------------------------------------------------
void DictMerger::convertDialInText()
{
	string sec_text;
	size_t pos;
	if(status == 1 && dicttools.size() == 2)
	{
		for(auto &elem_info : dicttools[0].getDict()[RecType::INFO])
		{
			sec_text = elem_info.second;
			for(auto &elem_dial : dicttools[1].getDict()[RecType::DIAL])
			{
				if(elem_dial.first.substr(5) != elem_dial.second)
				{
					string r = "\\b" + elem_dial.first.substr(5) + "\\b";
					regex re(r, regex_constants::icase);
					smatch found;
					regex_search(sec_text, found, re);
					if(!found[0].str().empty())
					{
						pos = found.position(0) + found[0].str().size();
						sec_text.insert(pos, " [" + elem_dial.second + "]");
					}
				}
			}
			dict[RecType::INFO].insert({elem_info.first, sec_text});
		}
	}
}
