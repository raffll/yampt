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
    void createEsm();

    bool add_hyperlinks = false;
    bool disable_annotations = false;

    std::vector<std::string> args;
    std::vector<std::string> file_paths;
    std::vector<std::string> dict_paths;
    std::string output = "MERGED.xml";
    std::string suffix;

    Tools::Encoding encoding = Tools::Encoding::UNKNOWN;
};

#endif // USERINTERFACE_HPP
