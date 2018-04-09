#ifndef ESMTOOLS_HPP
#define ESMTOOLS_HPP

#include "config.hpp"
#include "esmreader.hpp"

class EsmTools
{
public:
    std::string dumpFile();
    std::string dumpSCDT();
    std::string makeScriptList();

    std::string getNamePrefix() { return esm.getNamePrefix(); }

    EsmTools(const std::string &path);

private:
    EsmReader esm;
    Tools tools;
};

#endif // ESMTOOLS_HPP
