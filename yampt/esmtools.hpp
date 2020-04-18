#ifndef ESMTOOLS_HPP
#define ESMTOOLS_HPP

#include "config.hpp"
#include "esmreader.hpp"

class EsmTools
{
public:
    std::string dumpFile();
    std::string makeScriptList();
    static bool findChar(EsmReader & esm);

    std::string getNamePrefix() { return esm.getNamePrefix(); }

    EsmTools(const std::string &path);

private:
    EsmReader esm;
    Tools tools;

    static bool findChar(const std::string & friendly_text);
};

#endif // ESMTOOLS_HPP
