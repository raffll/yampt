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
void UserInterface::printMakeDictHeader()
{
    std::cout << "-----------------------------------------------" << std::endl
              << "          Created / Missing / Identical /   All" << std::endl
              << "-----------------------------------------------" << std::endl;
}

//----------------------------------------------------------
void UserInterface::makeDictRaw()
{
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i]);
        if(creator.getStatus() == true)
        {
            std::cout << "--> Start creating raw dictionary!" << std::endl;
            printMakeDictHeader();
            creator.makeDict();
            std::cout << "----------------------------------------------" << std::endl;
            tools.writeDict(creator.getDict(), creator.getNamePrefix() + ".RAW.xml");
        }
    }
}

//----------------------------------------------------------
void UserInterface::makeDictBase()
{
    DictCreator creator(file_path[0], file_path[1]);
    if(creator.getStatus() == true)
    {
        std::cout << "--> Start creating base dictionary!" << std::endl;
        std::cout << "--> Check dictionary for \"MISSING\" keyword!" << std::endl;
        std::cout << "    Missing CELL and DIAL records needs to be added manually!" << std::endl;
        printMakeDictHeader();
        creator.makeDict();
        std::cout << "----------------------------------------------" << std::endl;
        tools.writeDict(creator.getDict(), creator.getNamePrefix() + ".BASE.xml");
    }
}

//----------------------------------------------------------
void UserInterface::makeDictAll()
{
    std::string log;
    DictMerger merger(dict_path);
    merger.mergeDict();
    log += merger.getLog();
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i], merger, yampt::ins_mode::ALL);
        if(creator.getStatus() == true)
        {
            std::cout << "--> Start creating dictionary with all records!" << std::endl;
            printMakeDictHeader();
            creator.makeDict();
            std::cout << "----------------------------------------------" << std::endl;
            tools.writeDict(creator.getDict(), creator.getNamePrefix() + ".ALL.xml");
        }
    }
    tools.writeText(log, "yampt.log");
}

//----------------------------------------------------------
void UserInterface::makeDictNotFound()
{
    std::string log;
    DictMerger merger(dict_path);
    merger.mergeDict();
    log += merger.getLog();
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i], merger, yampt::ins_mode::NOTFOUND);
        if(creator.getStatus() == true)
        {
            std::cout << "--> Start creating dictionary with not found records!" << std::endl;
            printMakeDictHeader();
            creator.makeDict();
            std::cout << "----------------------------------------------" << std::endl;
            tools.writeDict(creator.getDict(), creator.getNamePrefix() + ".NOTFOUND.xml");
        }
    }
    tools.writeText(log, "yampt.log");
}

//----------------------------------------------------------
void UserInterface::makeDictChanged()
{
    std::string log;
    DictMerger merger(dict_path);
    merger.mergeDict();
    log += merger.getLog();
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        DictCreator creator(file_path[i], merger, yampt::ins_mode::CHANGED);
        if(creator.getStatus() == true)
        {
            std::cout << "--> Start creating dictionary with changed records!" << std::endl;
            printMakeDictHeader();
            creator.makeDict();
            std::cout << "----------------------------------------------" << std::endl;
            tools.writeDict(creator.getDict(), creator.getNamePrefix() + ".CHANGED.xml");
        }
    }
    tools.writeText(log, "yampt.log");
}

//----------------------------------------------------------
void UserInterface::mergeDict()
{
    std::string log;
    DictMerger merger(dict_path);
    merger.mergeDict();
    log += merger.getLog();
    tools.writeDict(merger.getDict(), output);
    tools.writeText(log, "yampt.log");
}

//----------------------------------------------------------
void UserInterface::convertEsm()
{
    std::string log;
    DictMerger merger(dict_path);
    merger.mergeDict();
    log += merger.getLog();
    tools.writeText(log, "yampt.log");

    for(size_t i = 0; i < file_path.size(); ++i)
    {
        EsmConverter converter(file_path[i], merger, add_dial);
        if(converter.getStatus() == true)
        {
            std::cout << "--> Start converting file!" << std::endl;
            std::cout << "----------------------------------------------" << std::endl
                      << "      Converted / Skipped / Unchanged /    All" << std::endl
                      << "----------------------------------------------" << std::endl;
            converter.convertEsm();
            std::cout << "----------------------------------------------" << std::endl;
            tools.writeFile(converter.getRecordColl(), converter.getNameFull());
        }
    }
}

//----------------------------------------------------------
void UserInterface::dumpFile()
{
    std::string text;
    for(size_t i = 0; i < file_path.size(); ++i)
    {
        EsmTools dump(file_path[i]);
        std::cout << "--> Start creating file dump!" << std::endl;
        text = dump.dumpFile();
        std::cout << "----------------------------------------------" << std::endl;
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
        std::cout << "--> Start creating script list!" << std::endl;
        text = dump.makeScriptList();
        std::cout << "----------------------------------------------" << std::endl;
        tools.writeText(text, dump.getNamePrefix() + ".SCRIPTS.txt");
    }
}
