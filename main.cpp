#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

#include "tools.hpp"
#include "creator.hpp"
#include "merger.hpp"
#include "converter.hpp"

int main(int argc, char *argv[])
{
	string comm;
	string name = argv[0];
	string usage = "Usage: " + name + " [command]"
				   "\n"
				   "\n  --help                              Print this message."
				   "\n  --make       [file1] <file2>        Make dictionary from esp/esm plugin."
				   "\n                                      Or make base dictionary from two localized esm files."
				   "\n  --compare    [dict1] [dict2]        Compare two dictionaries and create log."
				   "\n  --merge      [dict1]...             Merge dictionaries from paths and delete doubled records."
				   "\n  --convert    [file1] [dict1]...     Convert plugin from dictionaries in paths.\n";
	if(argc > 1)
	{
		comm = argv[1];
	}
	if(comm == "--help")
	{
		cout << usage;
	}
	else if(comm == "--make" && argc == 3)
	{
			creator c(argv[2]);
			c.makeDict();
			c.writeDict();
	}
	else if(comm == "--make" && argc == 4)
	{
			creator c(argv[2], argv[3]);
			c.makeDict();
			c.writeDict();
	}
	else if(comm == "--compare" && argc == 4)
	{
			merger m(argv[2], argv[3]);
			m.writeDiff();
			m.writeLog();
	}
	else if(comm == "--merge" && argc == 3)
	{
			merger m(argv[2]);
			m.mergeDict();
			m.writeMerged();
			m.writeLog();
	}
	else if(comm == "--merge" && argc == 4)
	{
			merger m(argv[2], argv[3]);
			m.mergeDict();
			m.writeMerged();
			m.writeLog();
	}
	else if(comm == "--merge" && argc == 5)
	{
			merger m(argv[2], argv[3], argv[4]);
			m.mergeDict();
			m.writeMerged();
			m.writeLog();
	}
	/*else if(comm == "--convert")
	{
		if(argc > 3)
		{
			vector<string> dict_path;
			for(int i = 3; i < argc; i++)
			{
				dict_path.push_back(argv[i]);
			}
			converter m(argv[2], dict_path);
			m.convertCell();
			m.convertGmst();
			m.convertFnam();
			m.convertDesc();
			m.writeEsm();
		}
	}*/
	else
	{
		cout << usage;
	}
}
