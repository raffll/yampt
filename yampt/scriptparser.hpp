#ifndef SCRIPTPARSER_EX_HPP
#define SCRIPTPARSER_EX_HPP

#include "includes.hpp"
#include "tools.hpp"
#include "dictmerger.hpp"

class ScriptParser
{
public:
    void convertScript();

    std::string getNewScript() { return new_script; }
    std::string getNewSCDT() { return new_SCDT; }

    ScriptParser(
        const Tools::RecType type,
        const DictMerger & merger,
        const std::string & script_name,
        const std::string & file_name,
        const std::string & old_script,
        const std::string & old_SCDT = "");

private:
    void convertLine(
        const std::string & keyword,
        const int pos_in_expression,
        const Tools::RecType type);
    void trimLine();
    void extractText(const int pos_in_expression);
    void removeQuotes();
    void findNewText(const Tools::RecType text_type);
    void insertNewText();
    void convertTextInCompiled(const bool is_getpccell);
    void convertLine();
    void findKeyword();
    void findNewMessage();
    void convertMessageInCompiled();
    std::vector<std::string> splitLine(const std::string & cur_line) const;
    void trimLastNewLineChars();
    void dumpError();
    void ReplaceVerticalLinesByNewLine(std::string & message);

    const Tools::RecType type;
    const DictMerger * merger;
    const std::string script_name;
    const std::string file_name;
    const std::string old_script;
    const std::string old_SCDT;

    std::string new_script;
    std::string new_SCDT;

    bool is_done = false;
    std::string line;
    std::string line_lc;
    std::string old_text;
    std::string new_line;
    std::string new_text;
    size_t pos = 0;
    size_t pos_c = 0;
    size_t keyword_pos = 0;
    std::string keyword;
    bool error = false;
};

#endif // SCRIPTPARSER_EX_HPP
