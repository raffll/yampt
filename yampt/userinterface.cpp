#include "userinterface.hpp"
#include "dictcreator.hpp"
#include "dictmerger.hpp"
#include "esmconverter.hpp"

//----------------------------------------------------------
UserInterface::UserInterface(std::vector<std::string> & arg)
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
    if (arg.size() > 1)
    {
        for (size_t i = 2; i < arg.size(); ++i)
        {
            if (arg[i] == "--add-hyperlinks")
            {
                add_hyperlinks = true;
            }
            else if (arg[i] == "--windows-1250")
            {
                encoding = Tools::Encoding::WINDOWS_1250;
            }
            else if (arg[i] == "-f")
            {
                command = "-f";
            }
            else if (arg[i] == "-d")
            {
                command = "-d";
            }
            else if (arg[i] == "-o")
            {
                command = "-o";
            }
            else if (arg[i] == "-s")
            {
                command = "-s";
            }
            else
            {
                if (command == "-f")
                {
                    file_path.push_back(arg[i]);
                }
                if (command == "-d")
                {
                    dict_path_reverse.push_back(arg[i]);
                }
                if (command == "-o")
                {
                    output = arg[i];
                }
                if (command == "-s")
                {
                    suffix = arg[i];
                }
            }
        }
    }

    if (output.empty())
    {
        output = "Merged.xml";
    }

    dict_path.insert(dict_path.begin(), dict_path_reverse.rbegin(), dict_path_reverse.rend());
}

//----------------------------------------------------------
void UserInterface::runCommand()
{
    if (arg.size() > 1)
    {
        if (arg[1] == "--make-raw" && file_path.size() > 0)
        {
            makeDictRaw();
        }
        else if (arg[1] == "--make-base" && file_path.size() == 2)
        {
            makeDictBase();
        }
        else if (arg[1] == "--make-all" && file_path.size() > 0 && dict_path.size() > 0)
        {
            makeDictAll();
        }
        else if (arg[1] == "--make-not" && file_path.size() > 0 && dict_path.size() > 0)
        {
            makeDictNotFound();
        }
        else if (arg[1] == "--make-changed" && file_path.size() > 0 && dict_path.size() > 0)
        {
            makeDictChanged();
        }
        else if (arg[1] == "--merge" && dict_path.size() > 0)
        {
            mergeDict();
        }
        else if (arg[1] == "--convert" && file_path.size() > 0 && dict_path.size() > 0)
        {
            convertEsm();
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
    for (size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i]);
        Tools::writeDict(creator.getDict(), creator.getName().prefix + ".RAW.xml");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
    DictCreator creator(file_path[0], file_path[1]);
    Tools::writeDict(creator.getDict(), creator.getName().prefix + ".BASE.xml");
}

//----------------------------------------------------------
void UserInterface::makeDictAll()
{
    DictMerger merger(dict_path);
    for (size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i], merger, Tools::CreatorMode::ALL);
        Tools::writeDict(creator.getDict(), creator.getName().prefix + ".ALL.xml");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictNotFound()
{
    DictMerger merger(dict_path);
    for (size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i], merger, Tools::CreatorMode::NOTFOUND);
        Tools::writeDict(creator.getDict(), creator.getName().prefix + ".NOTFOUND.xml");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictChanged()
{
    DictMerger merger(dict_path);
    for (size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i], merger, Tools::CreatorMode::CHANGED);
        Tools::writeDict(creator.getDict(), creator.getName().prefix + ".CHANGED.xml");
    }
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
    DictMerger merger(dict_path);
    Tools::writeDict(merger.getDict(), output);
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
    DictMerger merger(dict_path);
    for (size_t i = 0; i < file_path.size(); ++i)
    {
        EsmConverter converter(file_path[i], merger, add_hyperlinks, suffix, encoding);
        if (converter.isLoaded())
        {
            Tools::writeFile(converter.getRecordColl(), converter.getName().prefix + suffix + converter.getName().suffix);
            boost::filesystem::last_write_time(converter.getName().prefix + suffix + converter.getName().suffix,
                                               converter.getTime());
        }
    }
}
