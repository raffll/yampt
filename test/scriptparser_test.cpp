#include "scriptparser_test.hpp"

//----------------------------------------------------------
ScriptParserTest::ScriptParserTest(DictMerger &merger) :
    merger(&merger)
{

}

//----------------------------------------------------------
void ScriptParserTest::runTest()
{
    std::vector<std::string> input;
    std::vector<std::string> output;
    std::string line;

    std::ifstream file_i("input.txt");
    std::ifstream file_o("output.txt");
    if(file_i && file_o)
    {
        while(std::getline(file_i, line))
        {
            line = tools.eraseCarriageReturnChar(line);
            input.push_back(line);
        }
        while(std::getline(file_o, line))
        {
            line = tools.eraseCarriageReturnChar(line);
            output.push_back(line);
        }
    }

    if(input.size() == output.size())
    {
       std::cout << "OK" << std::endl;
    }

    for(size_t i = 0; i < input.size(); ++i)
    {
        if(input[i] != output[i])
        {
            ScriptParser parser(yampt::rec_type::SCTX, *merger, input[i]);
            parser.convertScript();
            if(parser.getNewFriendly() == output[i])
            {
                std::cout << "Test " << i << ": PASS" << std::endl;
            }
            else
            {
                std::cout << "Test " << i << ": FAIL" << std::endl;
                std::cout << input[i] << std::endl;
                std::cout << output[i] << std::endl;
                std::cout << parser.getNewFriendly() << std::endl;
            }
        }
    }
}
