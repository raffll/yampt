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
                annotations.add_hyperlinks = true;
            }
            else if (args[i] == "--add-annotations")
            {
                annotations.add_annotation = true;
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
        Tools::addLog("yampt v0.24\r\n");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictRaw()
{
    Tools::addLog("-> Start making \"RAW\" dictionaries...\r\n");
    for (size_t i = 0; i < file_paths.size(); ++i)
    {
        DictCreator creator(file_paths[i]);
        Tools::writeDict(creator.getDict(), creator.getName().name + ".RAW.xml");
    }
    Tools::addLog("-> Done!\r\n");
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
    Tools::addLog("-> Start making \"BASE\" dictionary...\r\n");
    DictCreator creator(file_paths[0], file_paths[1]);
    Tools::writeDict(creator.getDict(), creator.getName().name + ".BASE.xml", Tools::Save::BASE);
    Tools::writeDict(creator.getDict(), creator.getName().name + ".GLOS.xml", Tools::Save::GLOS);
    Tools::addLog("-> Done!\r\n");
}

//----------------------------------------------------------
void UserInterface::makeDictAll()
{
    Tools::addLog("-> Start making \"ALL\" dictionaries...\r\n");
    DictMerger merger(dict_paths);
    for (size_t i = 0; i < file_paths.size(); ++i)
    {
        DictCreator creator(file_paths[i], merger, Tools::CreatorMode::ALL, annotations);
        Tools::writeDict(creator.getDict(), creator.getName().name + ".ALL.xml");
    }
    Tools::addLog("-> Done!\r\n");
}

//----------------------------------------------------------
void UserInterface::makeDictNotFound()
{
    Tools::addLog("-> Start making \"NOTFOUND\" dictionaries...\r\n");
    DictMerger merger(dict_paths);
    for (size_t i = 0; i < file_paths.size(); ++i)
    {
        DictCreator creator(file_paths[i], merger, Tools::CreatorMode::NOTFOUND, annotations);
        Tools::writeDict(creator.getDict(), creator.getName().name + ".NOTFOUND.xml");
    }
    Tools::addLog("-> Done!\r\n");
}

//----------------------------------------------------------
void UserInterface::makeDictChanged()
{
    Tools::addLog("-> Start making \"CHANGED\" dictionaries...\r\n");
    DictMerger merger(dict_paths);
    for (size_t i = 0; i < file_paths.size(); ++i)
    {
        DictCreator creator(file_paths[i], merger, Tools::CreatorMode::CHANGED, annotations);
        Tools::writeDict(creator.getDict(), creator.getName().name + ".CHANGED.xml");
    }
    Tools::addLog("-> Done!\r\n");
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
    Tools::addLog("-> Start merging dictionaries...\r\n");
    DictMerger merger(dict_paths);
    Tools::writeDict(merger.getDict(), output);
    Tools::addLog("-> Done!\r\n");
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
    Tools::addLog("-> Start converting plugins...\r\n");
    DictMerger merger(dict_paths);
    for (const auto & file_path : file_paths)
    {
        EsmConverter converter(file_path, merger, annotations.add_hyperlinks, suffix, encoding, false);
        if (converter.isLoaded())
        {
            const auto & name = converter.getName().name + suffix + converter.getName().ext;
            Tools::writeFile(converter.getRecords(), name);
            boost::filesystem::last_write_time(name, converter.getTime());
        }
    }
    Tools::addLog("-> Done!\r\n");
}

//----------------------------------------------------------
void UserInterface::createEsm()
{
    Tools::addLog("-> Start creating plugins...\r\n");
    DictMerger merger(dict_paths);
    for (const auto & file_path : file_paths)
    {
        EsmConverter converter(file_path, merger, annotations.add_hyperlinks, suffix, encoding, true);
        if (converter.isLoaded())
        {
            const auto & name = converter.getName().name + ".CREATED" + converter.getName().ext;
            Tools::createFile(converter.getRecords(), name);
            boost::filesystem::last_write_time(name, converter.getTime() + 1);
        }
    }
    Tools::addLog("-> Done!\r\n");
}
