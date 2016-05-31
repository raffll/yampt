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
	string arg1;
	string arg2;
	string name = argv[0];
	string usage = "Usage: " + name + " [command]"
			"\n"
			"\n  --help                              Print this message."
			"\n  --make -a  [file1] [dict1]          Make dictionary from esp/esm plugin with all records."
			"\n  --make -d  [file1] [dict1]          Make without duplicated records from dictionary in path."
			"\n  --make -c  [file1] [dict1]          Make without converted records from dictionary in path."
			"\n  --make -r  [file1]                  Make without dial translation. Only for diagnostic."
			"\n  --make -b  [file1] [file2]          Make base dictionary from two localized esm files."
			"\n                                      This is required for future translations."
			"\n  --compare  [dict1] [dict2]          Compare two dictionaries and create differences log."
			"\n  --merge    [dict1] <dict2> <dict3>  Validate, merge and sort dictionaries from paths"
			"\n                                      and delete doubled records."
			"\n  --convert  [file1] [dict1] <dict2>  Convert plugin from dictionaries in paths."
			"\n                                      and create dictionary as \"--make -c\" command.\n";
	if(argc > 1)
	{
		arg1 = argv[1];
	}
	if(argc > 2)
	{
		arg2 = argv[2];
	}

	if(arg1 == "--help")
	{
		cout << usage;
	}
	else if(arg1 == "--make" && arg2 == "-r" && argc == 4)
	{
		creator c(argv[3]);
		c.makeDict();
		c.writeDict();
	}
	else if(arg1 == "--make" && arg2 == "-b" && argc == 5)
	{
		creator c(argv[3], argv[4]);
		c.makeDict();
		c.writeDict();
	}
	else if(arg1 == "--make" && arg2 == "-a" && argc == 5)
	{
		merger m(argv[4]);
		m.mergeDict();
		creator c(argv[3], m);
		c.makeDict();
		c.writeDict();
	}
	else if(arg1 == "--make" && arg2 == "-d" && argc == 5)
	{
		merger m(argv[4]);
		m.mergeDict();
		creator c(argv[3], m);
		c.makeDict();
		c.eraseDuplicates();
		c.writeDict();
	}
	else if(arg1 == "--make" && arg2 == "-c" && argc == 5)
	{
		merger m(argv[4]);
		m.mergeDict();
		creator c(argv[3], m);
		c.makeDict();
		c.eraseDuplicates();
		c.eraseDifferent();
		c.writeDict();
	}
	else if(arg1 == "--compare" && argc == 4)
	{
		merger m(argv[2], argv[3]);
		m.writeDiff();
		m.writeLog();
	}
	else if(arg1 == "--merge" && argc == 3)
	{
		merger m(argv[2]);
		m.mergeDict();
		m.writeLog();
		m.writeMerged();
	}
	else if(arg1 == "--merge" && argc == 4)
	{
		merger m(argv[3], argv[2]);
		m.mergeDict();
		m.writeLog();
		m.writeMerged();
	}
	else if(arg1 == "--merge" && argc == 5)
	{
		merger m(argv[4], argv[3], argv[2]);
		m.mergeDict();
		m.writeLog();
		m.writeMerged();
	}
	else if(arg1 == "--convert" && argc == 4)
	{
		merger m(argv[3]);
		m.mergeDict();
		m.writeLog();
		converter c(argv[2], m);
		c.convertEsm();
		c.writeEsm();
	}
	else if(arg1 == "--convert" && argc == 5)
	{
		merger m(argv[4], argv[3]);
		m.mergeDict();
		m.writeLog();
		converter c(argv[2], m);
		c.convertEsm();
		c.writeEsm();
	}
	else
	{
		cout << "Syntax error, try: " << argv[0] << " --help\n";
	}
}
