#ifndef ESMTOOLS_HPP
#define ESMTOOLS_HPP

#include "includes.hpp"
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

    static bool findChar(const std::string & friendly_text);
};

#endif // ESMTOOLS_HPP
