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
	int identical = 0;
	int duplicate = 0;
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
					duplicate++;
					log += dict_coll[i].getDictName() + "\r\n" +
					       elem.first + " --- " +
					       elem.second + "\r\n";
					log += dict_coll[i - 1].getDictName() + "\r\n" +
					       search->first + " >>> " +
					       search->second + "\r\n";
				}
				else
				{
					identical++;
				}
			}
		}
		if(dict_coll.size() == 1)
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
void Merger::writeMerged()
{
	if(status == 1)
	{
		string name = "yampt-merged.dic";
		ofstream file;
		file.open(name, ios::binary);
		for(const auto &elem : merged)
		{
			file << Config::sep[1] << elem.first
			     << Config::sep[2] << elem.second
			     << Config::sep[3] << "\r\n";
		}
		cerr << "--> Writing " << merged.size() << " records to " << name << "..." << endl;
	}
}

//----------------------------------------------------------
void Merger::writeDiff()
{
	if(status == 1)
	{
		string diff_pri;
		string diff_sec;
		for(auto &elem : dict_coll[0].getDict())
		{
			auto search = dict_coll[1].getDict().find(elem.first);
			if(search != dict_coll[1].getDict().end())
			{
				if(search->second != elem.second)
				{
					diff_pri += Config::sep[1] + elem.first + Config::sep[2] +
						    elem.second + Config::sep[3] + "\r\n";
					diff_sec += Config::sep[1] + search->first + Config::sep[2] +
					            search->second + Config::sep[3] + "\r\n";
				}
			}
		}
		if(!diff_pri.empty() && !diff_sec.empty())
		{
			string name_pri = "yampt-diff-0-" + dict_coll[0].getDictPrefix() + ".log";
			string name_sec = "yampt-diff-1-" + dict_coll[1].getDictPrefix() + ".log";
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
void Merger::writeLog()
{
	if(status == 1)
	{
		string name = "yampt.log";
		ofstream file;
		file.open(name, ios::binary);
		for(auto &elem : dict_coll)
		{
			log += elem.getDictLog();
		}
		file << log;
		cerr << "--> Writing " << name << "..." << endl;
	}
}
