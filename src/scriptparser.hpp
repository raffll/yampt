#ifndef SCRIPTPARSER_HPP
#define SCRIPTPARSER_HPP

#include "config.hpp"
#include "dictmerger.hpp"

class ScriptParser
{
public:
    void convertScript();

    std::string getNewFriendly() { return new_friendly; }
    std::string getNewCompiled() { return compiled_data; }

    ScriptParser();
    ScriptParser(const yampt::rec_type type,
                 DictMerger &merger,
                 const std::string &prefix,
                 const std::string &friendly_text);
    ScriptParser(const yampt::rec_type type,
                 DictMerger &merger,
                 const std::string &prefix,
                 const std::string &friendly_text,
                 const std::string &compiled_data);

private:
    std::string checkLine(const std::string &line,
                          const std::string &line_lc);
    std::string checkLine(const std::string &line,
                          const std::string &line_lc,
                          const std::string &keyword,
                          const int pos_in_expr,
                          const yampt::rec_type text_type,
                          const bool is_getpccell);
    std::string convertLine(const std::string &line);
    std::string findInnerTextInDict(const std::string &text,
                         const yampt::rec_type text_type);
    std::string convertInnerTextInLine(const std::string &line,
                            const std::string &text,
                            const size_t text_pos,
                            const std::string &new_text);
    void convertLineInCompiledScriptData(const std::string &line,
                                         const std::string &new_line,
                                         const size_t keyword_pos,
                                         const bool is_say);
    void convertInnerTextInCompiledScriptData(const std::string &text,
                                              const std::string &new_text,
                                              const bool is_getpccell);
    std::pair<std::string, size_t> extractInnerTextFromLine(const std::string &line,
                                               const size_t keyword_pos,
                                               const int pos_in_expression);
    std::vector<std::string> splitLine(const std::string &line,
                                       const size_t keyword_pos,
                                       const bool is_say);

    yampt::rec_type type;
    DictMerger *merger;
    Tools tools;

    const std::string prefix;
    const std::string friendly_text;
    std::string new_friendly;

    std::string compiled_data;
    size_t pos_in_compiled;

    bool is_done;
};

#endif // SCRIPTPARSER_HPP
