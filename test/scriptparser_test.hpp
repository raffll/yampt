#ifndef SCRIPTPARSER_TEST_HPP
#define SCRIPTPARSER_TEST_HPP

#include "../src/config.hpp"
#include "../src/dictmerger.hpp"
#include "../src/scriptparser.hpp"

class ScriptParserTest
{
public:
    void runTest();

    ScriptParserTest(DictMerger &merger);

private:
    DictMerger *merger;
    Tools tools;
};

#endif // SCRIPTPARSER_TEST_HPP
