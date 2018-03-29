#ifndef SCRIPTPARSER_TEST_HPP
#define SCRIPTPARSER_TEST_HPP

#include "config.hpp"
#include "scriptparser.hpp"

class ScriptParser_Test : public ScriptParser
{
public:
    ScriptParser_Test();

private:
    void splitLine_1_Test();
    void splitLine_2_Test();
    void extractText_1_Test();
    void extractText_2_Test();
};

#endif // SCRIPTPARSER_TEST_HPP
