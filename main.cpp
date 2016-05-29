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
	string usage = "Usage: yampt [command]"
				   "\n"
				   "\n  --help                              Print this message."
				   "\n  --make       [file1]                Make dictionary from esp/esm plugin."
				   "\n  --make-base  [file1] [file2]        Make base dictionary from two localized esm files."
				   "\n  --merge      [dict1] [dict2]...     Merge dictionaries from paths and delete doubled records."
				   "\n  --convert    [file1]... [dict1]...  Convert plugins from dictionaries in paths.\n";

	if(argc > 1)
	{
		comm = argv[1];
	}
	if(comm == "--help")
	{
		cout << usage;
	}
	else if(comm == "--make")
	{
		if(argc == 3)
		{
			creator c(argv[2]);
			c.makeDict();
			c.writeDict();
		}
	}
	else if(comm == "--make-base")
	{
		if(argc == 4)
		{
			creator c(argv[2], argv[3]);
			c.makeDict();
			c.writeDict();
		}
	}
	else if(comm == "--compare")
	{
		if(argc == 4)
		{
			merger m(argv[2], argv[3]);
			m.writeDiff();
		}
	}
	else if(comm == "--merge")
	{
		if(argc == 3)
		{
			merger m(argv[2]);
			m.mergeDict();
			m.writeMerged();
		}
		if(argc == 4)
		{
			merger m(argv[2], argv[3]);
			m.mergeDict();
			m.writeMerged();
		}
		if(argc == 5)
		{
			merger m(argv[2], argv[3], argv[4]);
			m.mergeDict();
			m.writeMerged();
		}
	}
	else if(comm == "--convert")
	{
		if(argc == 4)
		{
			converter m(argv[2], argv[3]);
			m.convertCell();
			m.writeFile();
		}
	}
	else
	{
		cout << usage;
	}
}
