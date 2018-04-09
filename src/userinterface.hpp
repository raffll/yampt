#ifndef USERINTERFACE_HPP
#define USERINTERFACE_HPP

#include "config.hpp"
#include "dictcreator.hpp"
#include "dictmerger.hpp"
#include "esmconverter.hpp"
#include "esmtools.hpp"

class UserInterface
{
public:
    UserInterface(std::vector<std::string> &arg);

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
    void dumpSCDT();
    void makeScriptList();

    Tools tools;

    bool add_dial = false;
    bool ext_log = false;

    std::vector<std::string> arg;
    std::vector<std::string> file_path;
    std::vector<std::string> dict_path;
    std::string output;
};

#endif // USERINTERFACE_HPP
