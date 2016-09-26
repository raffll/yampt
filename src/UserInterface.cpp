#include "UserInterface.hpp"

using namespace std;
using namespace yampt;

//----------------------------------------------------------
UserInterface::UserInterface(vector<string> &a)
{
	arg = a;
	string command;
	std::vector<std::string> dict_p_tmp;
	if(arg.size() > 1)
	{
		for(size_t i = 2; i < arg.size(); ++i)
		{
			if(arg[i] == "-a")
			{
				more_info = true;
			}
			else if(arg[i] == "-r")
			{
				replace_broken = true;
			}
			else if(arg[i] == "--add-dial")
			{
				add_dial = true;
			}
			else if(arg[i] == "--safe")
			{
				convert_safe = true;
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
					file_p.push_back(arg[i]);
				}
				if(command == "-d")
				{
					dict_p_tmp.push_back(arg[i]);
				}
			}
		}
	}

	dict_p.insert(dict_p.begin(), dict_p_tmp.rbegin(), dict_p_tmp.rend());

	if(arg.size() > 1)
	{
		if(arg[1] == "--make-raw" && file_p.size() > 0)
		{
			makeDictRaw();
		}
		else if(arg[1] == "--make-base" && file_p.size() == 2)
		{
			makeDictBase();
		}
		else if(arg[1] == "--make-all" && file_p.size() > 0)
		{
			makeDict();
		}
		else if(arg[1] == "--make-not" && file_p.size() > 0)
		{
			no_duplicates = true;
			makeDict();
		}
		else if(arg[1] == "--merge" && dict_p.size() > 0)
		{
			mergeDict();
		}
		else if(arg[1] == "--convert" && file_p.size() > 0 && dict_p.size() > 0)
		{
			convertEsm();
		}
		else if(arg[1] == "--scripts" && file_p.size() > 0)
		{
			makeScriptText();
		}
		else if(arg[1] == "--compare" && dict_p.size() == 2)
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
	for(size_t i = 0; i < file_p.size(); ++i)
	{
		DictCreator creator(file_p[i], replace_broken);
		creator.makeDict();
		writer.writeDict(creator.getDict(), creator.getName() + ".dic");
	}
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
	DictCreator creator(file_p[0], file_p[1], replace_broken);
	creator.compareEsm();
	creator.makeDict();
	writer.writeDict(creator.getDict(), creator.getName() + ".dic");
}

//----------------------------------------------------------
void UserInterface::makeDict()
{
	DictMerger merger(dict_p, more_info);
	merger.mergeDict();
	writer.writeText(merger.getLog(), "yampt-merger.log");
	for(size_t i = 0; i < file_p.size(); ++i)
	{
		DictCreator creator(file_p[i], merger, no_duplicates, replace_broken);
		creator.makeDict();
		writer.writeDict(creator.getDict(), creator.getName() + ".dic");
	}
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
	DictMerger merger(dict_p, more_info);
	merger.mergeDict();
	writer.writeDict(merger.getDict(), "Merged.dic");
	writer.writeText(merger.getLog(), "yampt-merger.log");
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
	string log;
	DictMerger merger(dict_p, more_info);
	merger.mergeDict();
	writer.writeText(merger.getLog(), "yampt-merger.log");
	for(size_t i = 0; i < file_p.size(); ++i)
	{
		EsmConverter converter(file_p[i], merger, convert_safe, add_dial, replace_broken);
		converter.convertEsm();
		converter.writeEsm();
		log += converter.getLog();
	}
	writer.writeText(log, "yampt-converter.log");
}

//----------------------------------------------------------
void UserInterface::makeScriptText()
{
	for(size_t i = 0; i < file_p.size(); ++i)
	{
		DictCreator creator(file_p[i], replace_broken);
		creator.makeScriptText();
		writer.writeText(creator.getScriptText(), creator.getName() + ".scpt");
	}
}

//----------------------------------------------------------
void UserInterface::makeDiff()
{
	DictMerger merger(dict_p, more_info);
	merger.makeDiff();
	writer.writeText(merger.getDiff(0), merger.getNamePrefix(0) + ".0.diff");
	writer.writeText(merger.getDiff(1), merger.getNamePrefix(1) + ".1.diff");
}
