#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

#include "tools.hpp"
#include "esmtools.hpp"
#include "dicttools.hpp"
#include "creator.hpp"

int main(int argc, char *argv[])
{
	string comm;
	string usage = "Usage:"
				   "\n  -h\tPrint this message"
				   "\n  -m\tMake dictionary: [path_to_file]"
				   "\n    \tOptional: [path_to_file_1] [path_to_file_2]"
				   "\n  -d\tLoad dictionary: [path_to_dict_folder]\n";

	if(argc > 1)
	{
		comm = argv[1];
	}

	if(comm == "-h")
	{
		cout << usage;
	}
	else if(comm == "-m")
	{
		if(argc == 3)
		{
			creator c(argv[2]);
			c.base.printStatus();
		}
		else if(argc == 4)
		{
			creator c(argv[2], argv[3]);
			c.base.printStatus();
			c.extd.printStatus();
		}
		else
		{
			cout << usage;
		}
	}
	else if(comm == "-d")
	{
		if(argc == 3)
		{
			dicttools d[10] = {{argv[2], 0}, {argv[2], 1}, {argv[2], 2}, {argv[2], 3}, {argv[2], 4},
							   {argv[2], 5}, {argv[2], 6}, {argv[2], 7}, {argv[2], 8}, {argv[2], 9}};


			for(int i = 0; i < 10; i++)
			{
				d[i].printStatus();
			}
		}
		else
		{
			cout << usage;
		}
	}
	else
	{
		cout << usage;
	}
}
