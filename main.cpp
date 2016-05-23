#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

#include "tools.hpp"
#include "creator.hpp"
#include "merger.hpp"

int main(int argc, char *argv[])
{
	tools::quiet = 0;

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
			c.writeDictAll();
		}
	}
	else if(comm == "--make-base")
	{
		if(argc == 4)
		{
			creator c(argv[2], argv[3]);
			c.writeDictAll();
		}
	}
	else if(comm == "--merge")
	{
		if(argc == 4)
		{
			merger m(argv[2], argv[3]);
			//m.mergeDict(8);
			//m.writeDict(8);
			m.findDuplicates(8);
		}
	}
	else
	{
		cout << usage;
	}
}
