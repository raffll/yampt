#include "UserInterface.hpp"

//----------------------------------------------------------
UserInterface::UserInterface(std::vector<std::string> &a)
{
	arg = a;
    std::string command;
	std::vector<std::string> dict_p_tmp;
	if(arg.size() > 1)
	{
		for(size_t i = 2; i < arg.size(); ++i)
		{
			if(arg[i] == "--add-dial")
			{
				add_dial = true;
			}
			else if(arg[i] == "--safe")
			{
				safe = true;
			}
			else if(arg[i] == "-f")
			{
				command = "-f";
			}
			else if(arg[i] == "-d")
			{
				command = "-d";
			}
			else if(arg[i] == "-o")
			{
				command = "-o";
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
				if(command == "-o")
				{
					output.push_back(arg[i]);
				}
			}
		}
	}

	if(output.empty())
	{
		output.push_back("yampt-merged.dic");
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
		else if(arg[1] == "--make-all" && file_p.size() > 0 && dict_p.size() > 0)
		{
			makeDictAll();
		}
		else if(arg[1] == "--make-not" && file_p.size() > 0 && dict_p.size() > 0)
		{
			makeDictNotFound();
		}
		else if(arg[1] == "--make-changed" && file_p.size() > 0 && dict_p.size() > 0)
		{
			makeDictChanged();
		}
		else if(arg[1] == "--merge" && dict_p.size() > 0)
		{
			mergeDict();
		}
		else if(arg[1] == "--convert" && file_p.size() > 0 && dict_p.size() > 0)
		{
			convertEsm();
		}
		else if(arg[1] == "--binary-dump" && file_p.size() > 0)
		{
			binaryDump();
		}
		else if(arg[1] == "--find-diff" && dict_p.size() == 2)
		{
			findDiff();
		}
		else if(arg[1] == "--word-list" && dict_p.size() == 1)
		{
			wordList();
		}
		else if(arg[1] == "--swap-records" && dict_p.size() == 1)
		{
			swapRecords();
		}
		else
		{
            std::cout << "Syntax error!" << std::endl;
		}
	}
	else
	{
        std::cout << "Syntax error!" << std::endl;
	}
}

//----------------------------------------------------------
void UserInterface::makeDictRaw()
{
	for(size_t i = 0; i < file_p.size(); ++i)
	{
		DictCreator creator(file_p[i]);
		creator.makeDict();
		writer.writeDict(creator.getDict(), creator.getNamePrefix() + ".RAW.dic");
	}
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
	DictCreator creator(file_p[0], file_p[1]);
	creator.makeDict();
	writer.writeDict(creator.getDict(), creator.getNamePrefix() + ".dic");
}

//----------------------------------------------------------
void UserInterface::makeDictAll()
{
    std::string log;

	DictMerger merger(dict_p);
	merger.mergeDict();

	log += merger.getLog();

	for(size_t i = 0; i < file_p.size(); ++i)
	{
        DictCreator creator(file_p[i], merger, yampt::ins_mode::ALL);
		creator.makeDict();
		writer.writeDict(creator.getDict(), creator.getNamePrefix() + ".ALL.dic");
	}

	writer.writeText(log, "yampt.log");
}

//----------------------------------------------------------
void UserInterface::makeDictNotFound()
{
    std::string log;

	DictMerger merger(dict_p);
	merger.mergeDict();

	log += merger.getLog();

	for(size_t i = 0; i < file_p.size(); ++i)
	{
        DictCreator creator(file_p[i], merger, yampt::ins_mode::NOTFOUND);
		creator.makeDict();
		writer.writeDict(creator.getDict(), creator.getNamePrefix() + ".NOTFOUND.dic");
	}

	writer.writeText(log, "yampt.log");
}

//----------------------------------------------------------
void UserInterface::makeDictChanged()
{
    std::string log;

	DictMerger merger(dict_p);
	merger.mergeDict();

	log += merger.getLog();

	for(size_t i = 0; i < file_p.size(); ++i)
	{
        DictCreator creator(file_p[i], merger, yampt::ins_mode::CHANGED);
		creator.makeDict();
		writer.writeDict(creator.getDict(), creator.getNamePrefix() + ".CHANGED.dic");
	}

	writer.writeText(log, "yampt.log");
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
    std::string log;

	DictMerger merger(dict_p);
	merger.mergeDict();

	log += merger.getLog();

	writer.writeDict(merger.getDict(), output[0]);
	writer.writeText(log, "yampt.log");
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
    std::string log;

	DictMerger merger(dict_p);
	merger.mergeDict();

	log += merger.getLog();

	for(size_t i = 0; i < file_p.size(); ++i)
	{
		EsmConverter converter(file_p[i], merger, safe, add_dial);
		converter.convertEsm();
		converter.writeEsm();
		log += converter.getLog();
	}

	writer.writeText(log, "yampt.log");
}

//----------------------------------------------------------
void UserInterface::binaryDump()
{
	for(size_t i = 0; i < file_p.size(); ++i)
	{
		DictCreator creator(file_p[i]);
		creator.makeBinaryDump();
		writer.writeText(creator.getBinaryDump(), creator.getNamePrefix() + ".BINARY.log");
	}
}

//----------------------------------------------------------
void UserInterface::findDiff()
{
	DictMerger merger(dict_p);
	merger.findDiff();

	writer.writeDict(merger.getDiff(0), merger.getNamePrefix(0) + ".DIFF-2.dic");
	writer.writeDict(merger.getDiff(1), merger.getNamePrefix(1) + ".DIFF-1.dic");
	writer.writeText(merger.getLog(), "yampt.log");
}

//----------------------------------------------------------
void UserInterface::wordList()
{
	DictMerger merger(dict_p);
	merger.wordList();

	writer.writeText(merger.getLog(), "yampt.log");
}

//----------------------------------------------------------
void UserInterface::swapRecords()
{
	DictMerger merger(dict_p);
	merger.swapRecords();

	writer.writeDict(merger.getDict(), merger.getNamePrefix(0) + ".SWAP.dic");
}
