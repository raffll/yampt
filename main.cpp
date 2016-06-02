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
			"\n  --make -d  [file1] [dict1]          Make without duplicated records from dictionary."
			"\n  --make -c  [file1] [dict1]          Make without records from dictionary."
			"\n  --make -r  [file1]                  Like \"--make -a\", but without dial translation."
			"\n                                      Only for diagnostic."
			"\n  --make -b  [file1] [file2]          Make base dictionary from two different localized esm files."
			"\n                                      Which is required for automatic translations."
			"\n  --compare  [dict1] [dict2]          Compare two dictionaries and create differences log."
			"\n  --merge    [dict1] <dict2> <dict3>  Validate, merge, sort and delete doubled records"
			"\n                                      from dictionaries."
			"\n  --convert  [file1] [dict1] <dict2>  Convert plugin from dictionaries in paths."
			"\n                                      and create dictionary as \"--make -c\" command.";
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
		cout << usage << endl;
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
		c.compareEsm();
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
		m.writeMerged();
		m.writeLog();
	}
	else if(arg1 == "--merge" && argc == 4)
	{
		merger m(argv[3], argv[2]);
		m.mergeDict();
		m.writeMerged();
		m.writeLog();
	}
	else if(arg1 == "--merge" && argc == 5)
	{
		merger m(argv[4], argv[3], argv[2]);
		m.mergeDict();
		m.writeMerged();
		m.writeLog();
	}
	else if(arg1 == "--convert" && argc == 4)
	{
		merger m(argv[3]);
		m.mergeDict();
		m.writeLog();
		converter c(argv[2], m);
		c.convertEsm();
		c.writeEsm();
		creator r(argv[2], m);
		r.makeDict();
		r.eraseDuplicates();
		r.eraseDifferent();
		r.writeDict(true);

	}
	else if(arg1 == "--convert" && argc == 5)
	{
		merger m(argv[4], argv[3]);
		m.mergeDict();
		m.writeLog();
		converter c(argv[2], m);
		c.convertEsm();
		c.writeEsm();
		creator r(argv[2], m);
		r.makeDict();
		r.eraseDuplicates();
		r.eraseDifferent();
		r.writeDict(true);
	}
	else
	{
		cout << "Syntax error!" << endl;
		cout << usage << endl;
	}
}
