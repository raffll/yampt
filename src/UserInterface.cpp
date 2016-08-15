#include "UserInterface.hpp"

using namespace std;

//----------------------------------------------------------
UserInterface::UserInterface(vector<string> &a)
{
	prepareUi(a);
}

//----------------------------------------------------------
void UserInterface::prepareUi(vector<string> &a)
{
	arg = a;
	string command = "NULL";
	for(size_t i = 0; i < arg.size(); ++i)
	{
		if(arg[i] == "-f")
		{
			command = "-f";
		}
		else if(arg[i] == "-d")
		{
			command = "-d";
		}
		else
		{
			if(command == "-f")
			{
				arg_file.push_back(arg[i]);
			}
			if(command == "-d")
			{
				arg_dict.push_back(arg[i]);
			}
		}
	}
	vector<string> arg_dict_rev_tmp(arg_dict.rbegin(), arg_dict.rend());
	arg_dict_rev = arg_dict_rev_tmp;
}

//----------------------------------------------------------
void UserInterface::makeDictRaw()
{
	Config config;
	for(size_t i = 0; i < arg_file.size(); ++i)
	{
		DictCreator creator(arg_file[i]);
		creator.makeDict();
		creator.writeDict();
	}
	Config::writeLog();
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
	Config config;
	if(arg_file.size() == 2)
	{
		DictCreator creator(arg_file[0], arg_file[1]);
		creator.compareEsm();
		creator.makeDict();
		creator.writeDict();
	}
	Config::writeLog();
}

//----------------------------------------------------------
void UserInterface::makeDictAll()
{
	Config config;
	DictMerger merger(arg_dict_rev);
	merger.mergeDict();
	for(size_t i = 0; i < arg_file.size(); ++i)
	{
		DictCreator creator(arg_file[i], merger);
		creator.makeDict();
		creator.writeDict();
	}
	Config::writeLog();
}

//----------------------------------------------------------
void UserInterface::makeDictNot()
{
	Config config;
	DictMerger merger(arg_dict_rev);
	merger.mergeDict();
	for(size_t i = 0; i < arg_file.size(); ++i)
	{
		DictCreator creator(arg_file[i], merger, true);
		creator.makeDict();
		creator.writeDict();
	}
	Config::writeLog();
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
	Config config;
	DictMerger merger(arg_dict_rev);
	merger.mergeDict();
	merger.writeDict();
	Config::writeLog();
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
	Config config;
	DictMerger merger(arg_dict_rev);
	merger.mergeDict();
	for(size_t i = 0; i < arg_file.size(); ++i)
	{
		EsmConverter converter(arg_file[i], merger);
		converter.convertEsm();
		converter.writeEsm();
		if(converter.getStatus() == 1)
		{
			DictCreator creator(arg_file[i], merger, true);
			creator.makeDict();
			creator.writeDict();
		}
	}
	Config::writeLog();
}

//----------------------------------------------------------
void UserInterface::writeScripts()
{
	Config config;
	for(size_t i = 0; i < arg_file.size(); ++i)
	{
		DictCreator creator(arg_file[i]);
		creator.writeScripts();
	}
	Config::writeLog();
}

//----------------------------------------------------------
void UserInterface::writeCompare()
{
	Config config;
	if(arg_dict.size() == 2)
	{
		DictMerger merger(arg_dict);
		merger.writeCompare();
	}
	Config::writeLog();
}
