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
	int identical = 0;
	int duplicate = 0;
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
					duplicate++;
					log += dict[i].getDictName() + "\r\n" +
					       elem.first + " --- " +
					       elem.second + "\r\n";
					log += dict[i - 1].getDictName() + "\r\n" +
					       search->first + " >>> " +
					       search->second + "\r\n";
				}
				else
				{
					identical++;
				}
			}
		}
		if(dict[1].getDictStatus() == 0 && dict[2].getDictStatus() == 0)
		{
			cerr << "--> Sorting complete!" << endl;
		}
		else
		{
			cerr << "--> Merging complete!" << endl;
			cerr << "    --> Records merged: " << merged.size() << endl;
			cerr << "    --> Duplicate records not merged: " << duplicate << endl;
			cerr << "    --> Identical records not merged: " << identical << endl;
		}
	}
}

//----------------------------------------------------------
void merger::writeMerged()
{
	if(status == 1)
	{
		string name = "yampt-merged.dic";
		ofstream file;
		file.open(name, ios::binary);
		for(const auto &elem : merged)
		{
			file << config::sep[1] << elem.first << config::sep[2] << elem.second << config::sep[3] << "\r\n";
		}
		cerr << "--> Writing " << merged.size() << " records to " << name << "..." << endl;
	}
}

//----------------------------------------------------------
void merger::writeDiff()
{
	if(status == 1)
	{
		string diff_pri;
		string diff_sec;
		for(auto &elem : dict[0].getDict())
		{
			auto search = dict[1].getDict().find(elem.first);
			if(search != dict[1].getDict().end())
			{
				if(search->second != elem.second)
				{
					diff_pri += config::sep[1] + elem.first + config::sep[2] +
						    elem.second + config::sep[3] + "\r\n";
					diff_sec += config::sep[1] + search->first + config::sep[2] +
					            search->second + config::sep[3] + "\r\n";
				}
			}
		}
		if(!diff_pri.empty() && !diff_sec.empty())
		{
			string name_pri = "yampt-diff-0-" + dict[0].getDictPrefix() + ".log";
			string name_sec = "yampt-diff-1-" + dict[1].getDictPrefix() + ".log";
			ofstream file_pri;
			ofstream file_sec;
			file_pri.open(name_pri, ios::binary);
			file_sec.open(name_sec, ios::binary);
			file_pri << diff_pri;
			file_sec << diff_sec;
			cerr << "--> Writing " << name_pri << "..." << endl;
			cerr << "--> Writing " << name_sec << "..." << endl;
		}
		else
		{
			cerr << "--> No differences between dictionaries!" << endl;
		}
	}
}

//----------------------------------------------------------
void merger::writeLog()
{
	if(status == 1)
	{
		string name = "yampt.log";
		ofstream file;
		file.open(name, ios::binary);
		file << dict[0].getDictLog() + dict[1].getDictLog() + dict[2].getDictLog() + log;
		cerr << "--> Writing " << name << "..." << endl;
	}
}
