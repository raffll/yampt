#include "DictTools.hpp"

using namespace std;

//----------------------------------------------------------
DictTools::DictTools()
{

}

//----------------------------------------------------------
void DictTools::writeDict(const array<map<string, string>, 11> &dict, string name)
{
	if(getSize(dict) > 0)
	{
		ofstream file(name, ios::binary);
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
		cout << "--> Writing " << to_string(getSize(dict)) <<
			" records to " << name << "...\r\n";
	}
	else
	{
		cout << "--> No records to make dictionary!\r\n";
	}
}

//----------------------------------------------------------
int DictTools::getSize(const array<map<string, string>, 11> &dict)
{
	int size = 0;
	for(auto const &elem : dict)
	{
		size += elem.size();
	}
	return size;
}

