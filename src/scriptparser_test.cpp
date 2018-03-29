#include "scriptparser_test.hpp"

//----------------------------------------------------------
ScriptParser_Test::ScriptParser_Test()
{
    splitLine_1_Test();
    splitLine_2_Test();
    extractText_1_Test();
    extractText_2_Test();
}

//----------------------------------------------------------
void ScriptParser_Test::splitLine_1_Test()
{
    std::cout << "Test splitLine() for MESSAGEBOX and CHOICE" << std::endl;

    const std::vector<std::string> arg = { "Keyword \"Text\" \"Yes\" \"No\"",
                                           "Keyword\t\"Text\" \"Yes\" \"No\"" };

    const std::vector<std::vector<std::string>> expected = { { "Text", "Yes", "No" },
                                                             { "Text", "Yes", "No" } };

    std::vector<std::string> result;
    for(size_t i = 0; i < arg.size(); ++i)
    {
        result = splitLine(arg[i], false);
        if(result == expected[i])
        {
            std::cout << "Test " << i << ": OK" << std::endl;
        }
        else
        {
            std::cout << "Test " << i << ": FAIL" << std::endl;
        }
    }
}

//----------------------------------------------------------
void ScriptParser_Test::splitLine_2_Test()
{
    std::cout << "Test splitLine() for SAY" << std::endl;

    const std::vector<std::string> arg = { "Keyword \"Sound file\" \"Text\"",
                                           "Keyword\t\"Sound file\" \"Text\"" };

    const std::vector<std::vector<std::string>> expected = { { "Text" },
                                                             { "Text" } };

    std::vector<std::string> result;
    for(size_t i = 0; i < arg.size(); ++i)
    {
        result = splitLine(arg[i], true);
        if(result == expected[i])
        {
            std::cout << "Test " << i << ": OK" << std::endl;
        }
        else
        {
            std::cout << "Test " << i << ": FAIL" << std::endl;
        }
    }
}

//----------------------------------------------------------
void ScriptParser_Test::extractText_1_Test()
{
    std::cout << "Test extractText() for ADDTOPIC, SHOWMAP and CENTERONCELL" << std::endl;

    std::vector<std::string> arg = { "Keyword, \"Text\"",
                                     "Keyword, Text",
                                     "Keyword,  Text",
                                     "Keyword,\tText",
                                     "Keyword,\t\tText",
                                     "Keyword,Text",
                                     "Keyword, Text ",
                                     "Keyword, Text\t" };

    std::vector<std::pair<std::string, size_t>> expected = { std::make_pair("Text", 10),
                                                             std::make_pair("Text", 9),
                                                             std::make_pair("Text", 10),
                                                             std::make_pair("Text", 9),
                                                             std::make_pair("Text", 10),
                                                             std::make_pair("Text", 8),
                                                             std::make_pair("Text", 9),
                                                             std::make_pair("Text", 9) };

    std::pair<std::string, size_t> result;
    for(size_t i = 0; i < arg.size(); ++i)
    {
        result = extractText(arg[i], 0, 0);
        if(result == expected[i])
        {
            std::cout << "Test " << i << ": OK" << std::endl;
        }
        else
        {
            std::cout << "Test " << i << ": FAIL" << std::endl;
        }
    }
}

//----------------------------------------------------------
void ScriptParser_Test::extractText_2_Test()
{
    std::cout << "Test extractText() for GETPCCELL" << std::endl;

    std::vector<std::string> arg = { "Keyword, \"Text\" == 1",
                                     "Keyword, Text == 1",
                                     "Keyword,  Text == 1",
                                     "Keyword,\tText == 1",
                                     "Keyword,\t\tText == 1",
                                     "Keyword,Text == 1",
                                     "Keyword, Text== 1",
                                     "Keyword, Text\t== 1" };

    std::vector<std::pair<std::string, size_t>> expected = { std::make_pair("Text", 10),
                                                             std::make_pair("Text", 9),
                                                             std::make_pair("Text", 10),
                                                             std::make_pair("Text", 9),
                                                             std::make_pair("Text", 10),
                                                             std::make_pair("Text", 8),
                                                             std::make_pair("Text", 9),
                                                             std::make_pair("Text", 9) };

    std::pair<std::string, size_t> result;
    for(size_t i = 0; i < arg.size(); ++i)
    {
        result = extractText(arg[i], 0, 0);
        if(result == expected[i])
        {
            std::cout << "Test " << i << ": OK" << std::endl;
        }
        else
        {
            std::cout << "Test " << i << ": FAIL" << std::endl;
        }
    }
}
