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
	string err = "Syntax error...";

	dicttools d[10] = {{argv[1], 0},
			   {argv[1], 1},
			   {argv[1], 2},
			   {argv[1], 3},
			   {argv[1], 4},
			   {argv[1], 5},
			   {argv[1], 6},
			   {argv[1], 7},
			   {argv[1], 8},
			   {argv[1], 9}};

	for(int i = 0; i < 10; i++)
	{
		d[i].printStatus();
	}

	/*if(argc > 1)
	{
		comm = argv[1];
	}

	if(comm == "make" )
	{
		if(argc == 3)
		{
			creator c(argv[2]);
		}
		else if(argc == 4)
		{
			creator c(argv[2], argv[3], 0);
		}
		else
		{
			cout << err;
		}
	}
	else if(comm == "make_extd" )
	{
		if(argc == 3)
		{
			creator c(argv[2]);
		}
		else if(argc == 4)
		{
			creator c(argv[2], argv[3], 1);
		}
		else
		{
			cout << err;
		}
	}
	else
	{
		cout << err;
	}*/
}
