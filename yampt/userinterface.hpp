#ifndef USERINTERFACE_HPP
#define USERINTERFACE_HPP

#include "includes.hpp"
#include "tools.hpp"

class UserInterface
{
public:
    UserInterface(std::vector<std::string> & arg);

private:
    void parseCommandLine();
    void runCommand();

    void makeDictRaw();
    void makeDictBase();
    void makeDictAll();
    void makeDictNotFound();
    void makeDictChanged();
    void mergeDict();
    void convertEsm();
    void dumpFile();
    void makeScriptList();

    bool add_hyperlinks = false;
    bool safe = false;

    std::vector<std::string> arg;
    std::vector<std::string> file_path;
    std::vector<std::string> dict_path;
    std::string output;
    std::string suffix;

    Tools::Encoding encoding = Tools::Encoding::UNKNOWN;
};

#endif // USERINTERFACE_HPP
