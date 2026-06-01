#pragma once

#include "includes.hpp"
#include "tools.hpp"

class UserInterface
{
public:
    UserInterface(std::vector<std::string> & arg);

private:
    void parseCommandLine();
    void runCommand();

    void make_dict_();
    void make_dict_Base();
    void mergeDict();
    void convertEsm();
    void createEsm();

    std::vector<std::string> args;
    std::vector<std::string> file_paths;
    std::vector<std::string> dict_paths;
    std::string output;
    std::string suffix;

    tools_t::Encoding encoding = tools_t::Encoding::UNKNOWN;
};
