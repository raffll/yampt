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
                 const std::string &friendly_text);
    ScriptParser(const yampt::rec_type type,
                 DictMerger &merger,
                 const std::string &friendly_text,
                 const std::string &compiled_data);

protected:
    std::string convertLine(const std::string &line,
                            const size_t pos,
                            const bool is_say);
    std::string convertInnerText(const std::string &line,
                                 const size_t keyword_pos,
                                 const int pos_in_expression,
                                 const bool is_getpccell,
                                 const yampt::rec_type inner_type);
    void convertLineInCompiledScriptData(const std::string &line,
                                         const std::string &new_line,
                                         const bool is_say);
    void convertInnerTextInCompiledScriptData(const std::string &inner_text,
                                              const std::string &new_inner,
                                              const bool is_getpccell);
    std::pair<std::string, size_t> extractText(const std::string &line,
                                               const size_t pos,
                                               const int pos_in_expression);
    std::vector<std::string> splitLine(const std::string &line,
                                       const bool is_say = false);

private:
    yampt::rec_type type;
    DictMerger *merger;
    Tools tools;

    std::string friendly_text;
    std::string new_friendly;

    std::string compiled_data;
    size_t pos_in_compiled;

    int counter_all;
    int counter_converted;
    int counter_unchanged;

    bool to_convert;
};

#endif // SCRIPTPARSER_HPP
