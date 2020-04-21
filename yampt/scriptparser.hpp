#ifndef SCRIPTPARSER_HPP
#define SCRIPTPARSER_HPP

#include "includes.hpp"
#include "tools.hpp"
#include "dictmerger.hpp"

class ScriptParser
{
public:
    void convertScript(const std::string & friendly_text);

    std::string getNewFriendly() { return new_friendly; }
    std::string getNewCompiled() { return new_compiled; }

    ScriptParser(
        const Tools::RecType type,
        const DictMerger & merger,
        const std::string & prefix,
        const std::string & friendly_text,
        const std::string & compiled_data);

private:
    void convertLine(
        const std::string & keyword,
        const int pos_in_expression,
        const Tools::RecType type);

    const DictMerger * merger;
    const std::string prefix;

    std::string new_friendly;
    std::string new_compiled;

    bool is_done = false;
    std::string line;
    std::string line_lc;
    std::string line_tr;
    std::string new_text;
    std::string new_line;
    size_t pos;
    size_t pos_c;
};

#endif // SCRIPTPARSER_HPP
