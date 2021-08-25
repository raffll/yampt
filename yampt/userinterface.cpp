#include "userinterface.hpp"
#include "dictcreator.hpp"
#include "dictmerger.hpp"
#include "esmconverter.hpp"

//----------------------------------------------------------
UserInterface::UserInterface(std::vector<std::string> & arg)
    : args(arg)
{
    parseCommandLine();
    runCommand();
}

//----------------------------------------------------------
void UserInterface::parseCommandLine()
{
    std::string command;
    std::vector<std::string> dict_path_reverse;
    if (args.size() > 1)
    {
        for (size_t i = 2; i < args.size(); ++i)
        {
            if (args[i] == "--add-hyperlinks")
            {
                add_hyperlinks = true;
            }
            else if (args[i] == "--windows-1250")
            {
                encoding = Tools::Encoding::WINDOWS_1250;
            }
            else if (args[i] == "-f")
            {
                command = "-f";
            }
            else if (args[i] == "-d")
            {
                command = "-d";
            }
            else if (args[i] == "-o")
            {
                command = "-o";
            }
            else if (args[i] == "-s")
            {
                command = "-s";
            }
            else
            {
                if (command == "-f")
                {
                    file_paths.push_back(args[i]);
                }
                if (command == "-d")
                {
                    dict_path_reverse.push_back(args[i]);
                }
                if (command == "-o")
                {
                    output = args[i];
                }
                if (command == "-s")
                {
                    suffix = args[i];
                }
            }
        }
    }

    if (output.empty())
    {
        output = "Merged.xml";
    }

    dict_paths.insert(dict_paths.begin(), dict_path_reverse.rbegin(), dict_path_reverse.rend());
}

//----------------------------------------------------------
void UserInterface::runCommand()
{
    if (args.size() > 1)
    {
        if (args[1] == "--make-raw" && file_paths.size() > 0)
        {
            makeDictRaw();
        }
        else if (args[1] == "--make-base" && file_paths.size() == 2)
        {
            makeDictBase();
        }
        else if (args[1] == "--make-all" && file_paths.size() > 0 && dict_paths.size() > 0)
        {
            makeDictAll();
        }
        else if (args[1] == "--make-not" && file_paths.size() > 0 && dict_paths.size() > 0)
        {
            makeDictNotFound();
        }
        else if (args[1] == "--make-changed" && file_paths.size() > 0 && dict_paths.size() > 0)
        {
            makeDictChanged();
        }
        else if (args[1] == "--merge" && dict_paths.size() > 0)
        {
            mergeDict();
        }
        else if (args[1] == "--convert" && file_paths.size() > 0 && dict_paths.size() > 0)
        {
            convertEsm();
        }
        else if (args[1] == "--create" && file_paths.size() > 0 && dict_paths.size() > 0)
        {
            createEsm();
        }
        else
        {
            Tools::addLog("Syntax error!\r\n");
        }
    }
    else
    {
        Tools::addLog("yampt v0.22\r\n");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictRaw()
{
    for (size_t i = 0; i < file_paths.size(); ++i)
    {
        DictCreator creator(file_paths[i]);
        Tools::writeDict(creator.getDict(), creator.getName().name + ".RAW.xml");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
    DictCreator creator(file_paths[0], file_paths[1]);
    Tools::writeDict(creator.getDict(), creator.getName().name + ".BASE.xml");
}

//----------------------------------------------------------
void UserInterface::makeDictAll()
{
    DictMerger merger(dict_paths);
    for (size_t i = 0; i < file_paths.size(); ++i)
    {
        DictCreator creator(file_paths[i], merger, Tools::CreatorMode::ALL);
        Tools::writeDict(creator.getDict(), creator.getName().name + ".ALL.xml");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictNotFound()
{
    DictMerger merger(dict_paths);
    for (size_t i = 0; i < file_paths.size(); ++i)
    {
        DictCreator creator(file_paths[i], merger, Tools::CreatorMode::NOTFOUND);
        Tools::writeDict(creator.getDict(), creator.getName().name + ".NOTFOUND.xml");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictChanged()
{
    DictMerger merger(dict_paths);
    for (size_t i = 0; i < file_paths.size(); ++i)
    {
        DictCreator creator(file_paths[i], merger, Tools::CreatorMode::CHANGED);
        Tools::writeDict(creator.getDict(), creator.getName().name + ".CHANGED.xml");
    }
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
    DictMerger merger(dict_paths);
    Tools::writeDict(merger.getDict(), output);
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
    DictMerger merger(dict_paths);
    for (const auto & file_path : file_paths)
    {
        EsmConverter converter(file_path, merger, add_hyperlinks, suffix, encoding, false);
        if (converter.isLoaded())
        {
            const auto & name = converter.getName().name + suffix + converter.getName().ext;
            Tools::writeFile(converter.getRecords(), name);
            boost::filesystem::last_write_time(name, converter.getTime());
        }
    }
}

//----------------------------------------------------------
void UserInterface::createEsm()
{
    DictMerger merger(dict_paths);
    for (const auto & file_path : file_paths)
    {
        EsmConverter converter(file_path, merger, add_hyperlinks, suffix, encoding, true);
        if (converter.isLoaded())
        {
            const auto & name = converter.getName().name + ".CREATED" + converter.getName().ext;
            Tools::createFile(converter.getRecords(), name);
            boost::filesystem::last_write_time(name, converter.getTime() + 1);
        }
    }
}
