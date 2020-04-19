#ifndef ESMTOOLS_HPP
#define ESMTOOLS_HPP

#include "includes.hpp"
#include "esmreader.hpp"

class EsmTools
{
public:
    std::string dumpFile();
    std::string makeScriptList();

    std::string getNamePrefix() { return esm.getNamePrefix(); }

    EsmTools(const std::string & path);

private:
    EsmReader esm;
};

#endif // ESMTOOLS_HPP
