#include "UserInterface.hpp"

using namespace std;
using namespace yampt;

//----------------------------------------------------------
UserInterface::UserInterface(vector<string> &a)
{
	arg = a;
	string command;
	std::vector<std::string> path_dict_n_tmp;
	if(arg.size() > 1)
	{
		for(size_t i = 2; i < arg.size(); ++i)
		{
			if(arg[i] == "-a")
			{
				Config::setAllowMoreInfo(true);
			}
			else if(arg[i] == "-r")
			{
				Config::setReplaceBrokenChars(true);
			}
			if(arg[i] == "--with-dial")
			{
				Config::setAddDialToInfo(true);
			}
			if(arg[i] == "--safe")
			{
				Config::setSafeConvert(true);
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
					path_dict_n_tmp.push_back(arg[i]);
				}
			}
		}
	}

	path_dict_n.insert(path_dict_n.begin(), path_dict_n_tmp.rbegin(), path_dict_n_tmp.rend());

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
		else if(arg[1] == "--merge" && path_dict_n.size() > 0)
		{
			mergeDict();
		}
		else if(arg[1] == "--convert" && path_esm.size() > 0 && path_dict_n.size() > 0)
		{
			convertEsm();
		}
		else if(arg[1] == "--scripts" && path_esm.size() > 0)
		{
			makeScriptText();
		}
		else if(arg[1] == "--compare" && path_dict_n.size() == 2)
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
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		DictCreator creator(path_esm[i]);
		creator.makeDict();
		Writer::writeDict(creator.getDict(), creator.getName() + ".dic");
	}
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
	DictCreator creator(path_esm[0], path_esm[1]);
	creator.compareEsm();
	creator.makeDict();
	Writer::writeDict(creator.getDict(), creator.getName() + ".dic");
}

//----------------------------------------------------------
void UserInterface::makeDictAll()
{
	DictMerger merger(path_dict_n);
	merger.mergeDict();
	Writer::writeText(merger.getLog(), "yampt.log");
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		DictCreator creator(path_esm[i], merger);
		creator.makeDict();
		Writer::writeDict(creator.getDict(), creator.getName() + ".dic");
	}
}

//----------------------------------------------------------
void UserInterface::makeDictNot()
{
	DictMerger merger(path_dict_n);
	merger.mergeDict();
	Writer::writeText(merger.getLog(), "yampt.log");
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		DictCreator creator(path_esm[i], merger);
		creator.setNoDuplicates(true);
		creator.makeDict();
		Writer::writeDict(creator.getDict(), creator.getName() + ".dic");
	}
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
	DictMerger merger(path_dict_n);
	merger.mergeDict();
	Writer::writeDict(merger.getDict(), "Merged.dic");
	Writer::writeText(merger.getLog(), "yampt.log");
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
	DictMerger merger(path_dict_n);
	merger.mergeDict();
	Writer::writeText(merger.getLog(), "yampt.log");
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		EsmConverter converter(path_esm[i], merger);
		converter.convertEsm();
		converter.writeEsm();
	}
}

//----------------------------------------------------------
void UserInterface::makeScriptText()
{
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		DictCreator creator(path_esm[i]);
		creator.makeScriptText();
		Writer::writeText(creator.getScriptText(), creator.getName() + ".scpt");
	}
}

//----------------------------------------------------------
void UserInterface::makeDiff()
{
	DictMerger merger(path_dict_n);
	merger.makeDiff();
	Writer::writeText(merger.getDiff(0), merger.getNamePrefix(0) + ".0.diff");
	Writer::writeText(merger.getDiff(1), merger.getNamePrefix(1) + ".1.diff");
}
