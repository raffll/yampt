#include "UserInterface.hpp"

using namespace std;

//----------------------------------------------------------
UserInterface::UserInterface(vector<string> &a)
{
	arg = a;
	string command;
	std::vector<std::string> path_dict_n_tmp;
	std::vector<std::string> path_dict_f_tmp;
	if(arg.size() > 1)
	{
		for(size_t i = 2; i < arg.size(); ++i)
		{
			if(arg[i] == "-a")
			{
				cout << "--> Allow more than 512 characters: enabled" << endl;
				option["-a"] = 1;
			}
			else if(arg[i] == "-r")
			{
				cout << "--> Replace broken characters: enabled" << endl;
				option["-r"] = 1;
			}
			if(arg[i] == "--with-dial")
			{
				cout << "--> Add dialog topic names to INFO records: enabled" << endl;
				option["--with-dial"] = 1;
			}
			if(arg[i] == "--safe")
			{
				cout << "--> Safe convert: enabled" << endl;
				option["--safe"] = 1;
			}
			else if(arg[i] == "-f")
			{
				command = "-f";
			}
			else if(arg[i] == "-d")
			{
				command = "-d";
			}
			else if(arg[i] == "-e")
			{
				command = "-e";
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
				if(command == "-e")
				{
					path_dict_f_tmp.push_back(arg[i]);
				}
			}
		}
	}

	path_dict_n.insert(path_dict_n.begin(), path_dict_n_tmp.rbegin(), path_dict_n_tmp.rend());
	path_dict_f.insert(path_dict_f.begin(), path_dict_f_tmp.rbegin(), path_dict_f_tmp.rend());

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
			makeDict();
		}
		else if(arg[1] == "--make-not" && path_esm.size() > 0)
		{
			option["--no-duplicates"] = true;
			makeDict();
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
		config.writeDict(creator.getDict(), creator.getName() + ".dic");
	}
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
	DictCreator creator(path_esm[0], path_esm[1]);
	creator.compareEsm();
	creator.makeDict();
	config.writeDict(creator.getDict(), creator.getName() + ".dic");
}

//----------------------------------------------------------
void UserInterface::makeDict()
{
	DictMerger merger_n(path_dict_n, option["-a"]);
	merger_n.mergeDict();
	config.writeText(merger_n.getLog(), "yampt.log");
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		DictCreator creator(path_esm[i], merger_n);
		creator.setNoDuplicates(option["--no_duplicates"]);
		creator.makeDict();
		config.writeDict(creator.getDict(), creator.getName() + ".dic");
	}
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
	DictMerger merger_n(path_dict_n, option["-a"]);
	merger_n.mergeDict();
	config.writeDict(merger_n.getDict(), "Merged.dic");
	config.writeText(merger_n.getLog(), "yampt.log");
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
	DictMerger merger_n(path_dict_n, option["-a"]);
	merger_n.mergeDict();
	DictMerger merger_f(path_dict_f, option["-a"]);
	merger_f.mergeDict();
	config.writeText(merger_n.getLog(), "yampt.log");
	for(size_t i = 0; i < path_esm.size(); ++i)
	{
		EsmConverter converter(path_esm[i], merger_n, merger_f);
		converter.setAddDialToInfo(option["--with-dial"]);
		converter.setSafeConvert(option["--safe"]);
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
		config.writeText(creator.getScriptText(), creator.getName() + ".scpt");
	}
}

//----------------------------------------------------------
void UserInterface::makeDiff()
{
	DictMerger merger_n(path_dict_n, option["-a"]);
	merger_n.makeDiff();
	config.writeText(merger_n.getDiff(0), merger_n.getNamePrefix(0) + ".0.diff");
	config.writeText(merger_n.getDiff(1), merger_n.getNamePrefix(1) + ".1.diff");
}
