#include "UserInterface.hpp"

using namespace std;

//----------------------------------------------------------
UserInterface::UserInterface(vector<string> &a)
{
	arg = a;
	string command;

	if(arg.size() > 1)
	{
		for(size_t i = 2; i < arg.size(); ++i)
		{
			if(arg[i] == "-a")
			{
				cout << "--> Allow more than 512 chars: enabled" << endl;
				Config::setAllowMoreInfo(1);

			}
			else if(arg[i] == "-r")
			{
				cout << "--> Replace broken chars: enabled" << endl;
				Config::setReplaceBrokenChars(1);
			}
			else if(arg[i] == "-f")
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
	}
	vector<string> path_dict_rev_tmp(path_dict.rbegin(), path_dict.rend());
	path_dict_rev = path_dict_rev_tmp;

	if(arg.size() > 1)
	{
		if(arg[1] == "--make-raw" && path_esm.size() > 0)
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
			convertEsmWithDIAL();
		}
		else if(arg[1] == "--convert-safe" && path_esm.size() > 0 && path_dict_rev.size() > 0)
		{
			convertEsmSafe();
		}
		else if(arg[1] == "--scripts" && path_esm.size() > 0)
		{
			makeScriptText();
		}
		else if(arg[1] == "--compare" && path_dict.size() == 2)
		{
			makeDiff();
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
	config.writeText(merger.getLog(), "yampt.log");
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
	config.writeText(merger.getLog(), "yampt.log");
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
	config.writeText(merger.getLog(), "yampt.log");
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
	Config config;
	DictMerger merger(path_dict_rev);
	merger.mergeDict();
	config.writeText(merger.getLog(), "yampt.log");
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		EsmConverter converter(path_esm[i], merger);
		converter.convertEsm();
		converter.writeEsm();
	}
}

//----------------------------------------------------------
void UserInterface::convertEsmWithDIAL()
{
	Config config;
	DictMerger merger(path_dict_rev);
	merger.mergeDict();
	config.writeText(merger.getLog(), "yampt.log");
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		EsmConverter converter(path_esm[i], merger);
		converter.convertEsmWithDIAL();
		converter.writeEsm();
	}
}

//----------------------------------------------------------
void UserInterface::convertEsmSafe()
{
	Config config;
	DictMerger merger(path_dict_rev);
	merger.mergeDict();
	config.writeText(merger.getLog(), "yampt.log");
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		EsmConverter converter(path_esm[i], merger);
		converter.convertEsmSafe();
		converter.writeEsm();
	}
}

//----------------------------------------------------------
void UserInterface::makeScriptText()
{
	Config config;
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		DictCreator creator(path_esm[i]);
		creator.makeScriptText();
		config.writeText(creator.getScriptText(), creator.getName() + ".scpt");
	}
}

//----------------------------------------------------------
void UserInterface::makeDiff()
{
	Config config;
	DictMerger merger(path_dict);
	merger.makeDiff();
	config.writeText(merger.getDiff(0), merger.getNamePrefix(0) + ".0.diff");
	config.writeText(merger.getDiff(1), merger.getNamePrefix(1) + ".1.diff");
}
