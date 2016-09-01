#include "UserInterface.hpp"

using namespace std;

//----------------------------------------------------------
UserInterface::UserInterface(vector<string> &a)
{
	arg = a;
	string command;
	string usage = "Usage: " + arg[0] + " [command]"
		       "\n"
		       "\n  --help                                                Print this message."
		       "\n  --make-raw            -f <file_list>                  Make dictionary from esp/esm"
		       "\n                                                        plugins with all records."
		       "\n  --make-all            -f <file_list> -d <dict_list>   Make as above, but convert"
		       "\n                                                        INFO id with DIAL records"
		       "\n                                                        from selected dictionaries."
		       "\n  --make-not            -f <file_list> -d <dict_list>   Make without records from"
		       "\n                                                        selected dictionaries."
		       "\n  --make-base           -f [file_native] [file_foreign] Make base dictionary from"
		       "\n                                                        native and foreign"
		       "\n                                                        esm master files."
		       "\n  --merge               -d <dict_list>                  Validate, merge, sort"
		       "\n                                                        and delete doubled records."
		       "\n  --convert             -f <file_list> -d <dict_list>   Convert plugin from"
		       "\n                                                        selected dictionaries."
		       "\n  --convert-with-dial   -f <file_list> -d <dict_list>   Convert plugin from"
		       "\n                                                        selected dictionaries"
		       "\n                                                        and add dialog topic"
		       "\n                                                        names to end of not"
		       "\n                                                        converted INFO strings.";

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
				path_esm.push_back(arg[i]);
			}
			if(command == "-d")
			{
				path_dict.push_back(arg[i]);
			}
		}
	}
	vector<string> path_dict_rev_tmp(path_dict.rbegin(), path_dict.rend());
	path_dict_rev = path_dict_rev_tmp;

	if(arg.size() > 1)
	{
		if(arg[1] == "--help")
		{
			cout << usage << endl;
		}
		else if(arg[1] == "--make-raw" && path_esm.size() > 0)
		{
			makeDictRaw();
		}
		else if(arg[1] == "--make-base" && path_esm.size() == 2)
		{
			makeDictBase();
		}
		else if(arg[1] == "--make-all" && path_esm.size() > 0)
		{
			makeDictAll();
		}
		else if(arg[1] == "--make-not" && path_esm.size() > 0)
		{
			makeDictNot();
		}
		else if(arg[1] == "--merge" && path_dict_rev.size() > 0)
		{
			mergeDict();
		}
		else if(arg[1] == "--convert" && path_esm.size() > 0 && path_dict_rev.size() > 0)
		{
			convertEsm();
		}
		else if(arg[1] == "--convert-with-dial" && path_esm.size() > 0 && path_dict_rev.size() > 0)
		{
			convertEsmWithDial();
		}
		else
		{
			cout << "Syntax error!" << endl;
		}
	}
	else
	{
		cout << "Syntax error!" << endl;
	}
}

//----------------------------------------------------------
void UserInterface::makeDictRaw()
{
	Config config;
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		DictCreator creator(path_esm[i]);
		creator.makeDict();
		config.writeDict(creator.getDict(), creator.getName() + ".dic");
	}
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
	Config config;
	DictCreator creator(path_esm[0], path_esm[1]);
	creator.compareEsm();
	creator.makeDict();
	config.writeDict(creator.getDict(), creator.getName() + ".dic");
}

//----------------------------------------------------------
void UserInterface::makeDictAll()
{
	Config config;
	DictMerger merger(path_dict_rev);
	merger.mergeDict();
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		DictCreator creator(path_esm[i], merger);
		creator.makeDict();
		config.writeDict(creator.getDict(), creator.getName() + ".dic");
	}
}

//----------------------------------------------------------
void UserInterface::makeDictNot()
{
	Config config;
	DictMerger merger(path_dict_rev);
	merger.mergeDict();
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		DictCreator creator(path_esm[i], merger);
		creator.setNoDuplicates();
		creator.makeDict();
		config.writeDict(creator.getDict(), creator.getName() + ".dic");
	}
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
	Config config;
	DictMerger merger(path_dict_rev);
	merger.mergeDict();
	config.writeDict(merger.getDict(), "Merged.dic");
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
	Config config;
	DictMerger merger(path_dict_rev);
	merger.mergeDict();
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		EsmConverter converter(path_esm[i], merger);
		converter.convertEsm();
		converter.writeEsm();
	}
}

//----------------------------------------------------------
void UserInterface::convertEsmWithDial()
{
	Config config;
	DictMerger merger(path_dict_rev);
	merger.mergeDict();
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		EsmConverter converter(path_esm[i], merger);
		converter.convertEsmWithDial();
		converter.writeEsm();
	}
}
