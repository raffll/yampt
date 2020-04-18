#include "userinterface.hpp"

//----------------------------------------------------------
UserInterface::UserInterface(std::vector<std::string> &arg)
    : arg(arg)
{
    parseCommandLine();
    runCommand();
}

//----------------------------------------------------------
void UserInterface::parseCommandLine()
{
    std::string command;
    std::vector<std::string> dict_path_reverse;
    if(arg.size() > 1)
    {
        for(size_t i = 2; i < arg.size(); ++i)
        {
            if(arg[i] == "-a")
            {
                add_dial = true;
            }
            else if(arg[i] == "-l")
            {
                ext_log = true;
            }
            else if(arg[i] == "--safe")
            {
                safe_mode = Tools::safe_mode::enabled;
            }
            else if (arg[i] == "--safe-auto")
            {
                safe_mode = Tools::safe_mode::heuristic;
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
            else if(arg[i] == "-s")
            {
                command = "-s";
            }
            else
            {
                if(command == "-f")
                {
                    file_path.push_back(arg[i]);
                }
                if(command == "-d")
                {
                    dict_path_reverse.push_back(arg[i]);
                }
                if(command == "-o")
                {
                    output = arg[i];
                }
                if(command == "-s")
                {
                    suffix = arg[i];
                }
            }
        }
    }

    if(output.empty())
    {
        output = "Merged.xml";
    }

    dict_path.insert(dict_path.begin(), dict_path_reverse.rbegin(), dict_path_reverse.rend());
}

//----------------------------------------------------------
void UserInterface::runCommand()
{
    if(arg.size() > 1)
    {
        if(arg[1] == "--make-raw" && file_path.size() > 0)
        {
            makeDictRaw();
        }
        else if(arg[1] == "--make-base" && file_path.size() == 2)
        {
            makeDictBase();
        }
        else if(arg[1] == "--make-all" && file_path.size() > 0 && dict_path.size() > 0)
        {
            makeDictAll();
        }
        else if(arg[1] == "--make-not" && file_path.size() > 0 && dict_path.size() > 0)
        {
            makeDictNotFound();
        }
        else if(arg[1] == "--make-changed" && file_path.size() > 0 && dict_path.size() > 0)
        {
            makeDictChanged();
        }
        else if(arg[1] == "--merge" && dict_path.size() > 0)
        {
            mergeDict();
        }
        else if(arg[1] == "--convert" && file_path.size() > 0 && dict_path.size() > 0)
        {
            convertEsm();
        }
        else if(arg[1] == "--binary-dump" && file_path.size() > 0)
        {
            dumpFile();
        }
        else if(arg[1] == "--script-list" && file_path.size() > 0)
        {
            makeScriptList();
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
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i]);
        tools.writeDict(creator.getDict(), creator.getNamePrefix() + ".RAW.xml");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
    DictCreator creator(file_path[0], file_path[1]);
    tools.writeDict(creator.getDict(), creator.getNamePrefix() + ".BASE.xml");
}

//----------------------------------------------------------
void UserInterface::makeDictAll()
{
    DictMerger merger(dict_path, ext_log);
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i], merger, Tools::CreatorMode::ALL, false);
        tools.writeDict(creator.getDict(), creator.getNamePrefix() + ".ALL.xml");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictNotFound()
{
    DictMerger merger(dict_path, ext_log);
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i], merger, Tools::CreatorMode::NOTFOUND, add_dial);
        tools.writeDict(creator.getDict(), creator.getNamePrefix() + ".NOTFOUND.xml");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictChanged()
{
    DictMerger merger(dict_path, ext_log);
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i], merger, Tools::CreatorMode::CHANGED, add_dial);
        tools.writeDict(creator.getDict(), creator.getNamePrefix() + ".CHANGED.xml");
    }
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
    DictMerger merger(dict_path, ext_log);
    tools.writeDict(merger.getDict(), output);
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
    DictMerger merger(dict_path, ext_log);
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        EsmConverter converter(file_path[i], merger, add_dial, suffix, safe_mode);
        tools.writeFile(converter.getRecordColl(), converter.getNamePrefix() + suffix + converter.getNameSuffix());
        boost::filesystem::last_write_time(converter.getNamePrefix() + suffix + converter.getNameSuffix(),
                                           converter.getTime());
    }
}

//----------------------------------------------------------
void UserInterface::dumpFile()
{
    std::string text;
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        EsmTools dump(file_path[i]);
        text = dump.dumpFile();
        tools.writeText(text, dump.getNamePrefix() + ".DUMP.txt");
    }
}

//----------------------------------------------------------
void UserInterface::makeScriptList()
{
    std::string text;
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        EsmTools dump(file_path[i]);
        text = dump.makeScriptList();
        tools.writeText(text, dump.getNamePrefix() + ".SCRIPTS.txt");
    }
}
