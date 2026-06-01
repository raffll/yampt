#include "userinterface.hpp"
#include "dictcreator.hpp"
#include "dictmerger.hpp"
#include "dictreader.hpp"
#include "dictwriter.hpp"
#include "esmconverter.hpp"


UserInterface::UserInterface(std::vector<std::string> & arg)
    : args(arg)
{
    parseCommandLine();
    runCommand();
}


void UserInterface::parseCommandLine()
{
    std::string command;
    std::vector<std::string> dict_path_reverse;
    if (args.size() > 1)
    {
        for (size_t i = 2; i < args.size(); ++i)
        {
            if (args[i] == "--windows-1250")
            {
                encoding = tools_t::Encoding::WINDOWS_1250;
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


void UserInterface::runCommand()
{
    if (args.size() > 1)
    {
        if (args[1] == "--make" && file_paths.size() > 0)
        {
            make_dict_();
        }
        else if (args[1] == "--make-base" && file_paths.size() == 2)
        {
            make_dict_Base();
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
            tools_t::addLog("Syntax error!\r\n");
        }
    }
    else
    {
        tools_t::addLog("yampt v0.25\r\n");
    }
}


void UserInterface::make_dict_()
{
    tools_t::addLog("-> Start making dictionaries...\r\n");

    const tools_t::dict_t * base_dict = nullptr;
    tools_t::dict_t base_dict_storage;
    if (dict_paths.size() > 0)
    {
        DictReader reader(dict_paths[0]);
        if (reader.isLoaded())
        {
            base_dict_storage = reader.getDict();
            base_dict = &base_dict_storage;
        }
    }

    for (size_t i = 0; i < file_paths.size(); ++i)
    {
        dict_creator_t creator(file_paths[i], base_dict);

        std::string out_path;
        if (!output.empty())
        {
            out_path = output;
        }
        else
        {
            out_path = creator.get_name().name + ".json";
        }

        DictWriter::write(creator.get_dict(), out_path);
    }

    tools_t::addLog("-> Done!\r\n");
}


void UserInterface::make_dict_Base()
{
    tools_t::addLog("-> Start making \"BASE\" dictionary...\r\n");
    dict_creator_t creator(file_paths[0], file_paths[1]);
    DictWriter::write(creator.get_dict(), creator.get_name().name + ".BASE.json");
    tools_t::addLog("-> Done!\r\n");
}


void UserInterface::mergeDict()
{
    if (output.empty())
    {
        tools_t::addLog("Error: --merge requires -o <output_path>\r\n");
        return;
    }

    for (const auto & path : dict_paths)
    {
        const auto ext_pos = path.rfind('.');
        if (ext_pos != std::string::npos)
        {
            std::string ext = path.substr(ext_pos);
            for (auto & c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (ext == ".xml")
            {
                tools_t::addLog("Error: .xml dictionary files are no longer supported: " + path + "\r\n");
                return;
            }
        }
    }

    tools_t::addLog("-> Start merging dictionaries...\r\n");
    DictMerger merger(dict_paths);
    DictWriter::write(merger.getDict(), output);
    tools_t::addLog("-> Done!\r\n");
}


void UserInterface::convertEsm()
{
    tools_t::addLog("-> Start converting plugins...\r\n");
    DictMerger merger(dict_paths);
    for (const auto & file_path : file_paths)
    {
        EsmConverter converter(file_path, merger, false, suffix, encoding, false);
        if (converter.isLoaded())
        {
            const auto & name = converter.getName().name + suffix + converter.getName().ext;
            tools_t::writeFile(converter.getRecords(), name);
            std::filesystem::last_write_time(name, converter.getTime());
        }
    }
    tools_t::addLog("-> Done!\r\n");
}


void UserInterface::createEsm()
{
    tools_t::addLog("-> Start creating plugins...\r\n");
    DictMerger merger(dict_paths);
    for (const auto & file_path : file_paths)
    {
        EsmConverter converter(file_path, merger, false, suffix, encoding, true);
        if (converter.isLoaded())
        {
            const auto & name = converter.getName().name + ".CREATED" + converter.getName().ext;
            tools_t::createFile(converter.getRecords(), name);
            std::filesystem::last_write_time(name, converter.getTime() + std::chrono::seconds(1));
        }
    }
    tools_t::addLog("-> Done!\r\n");
}
