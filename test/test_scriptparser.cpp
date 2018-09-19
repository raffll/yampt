#include <catch2/catch.hpp>
#include "../src/config.hpp"
#include "../src/dictmerger.hpp"
#include "../src/scriptparser.hpp"

using namespace std;

TEST_CASE("AddTopic keyword")
{
    DictMerger merger;
    merger.addRecord(yampt::rec_type::DIAL, "Test Test", "Result Result");
    vector<std::string> lines;
    lines.push_back("AddTopic \"Test Test\"");
    lines.push_back("AddTopic \"Test Test\" ");
    lines.push_back("AddTopic\"Test Test\"");
    lines.push_back("AddTopic\"Test Test\" ");
    lines.push_back("AddTopic,\"Test Test\"");
    lines.push_back("AddTopic,\"Test Test\" ");
    lines.push_back("AddTopic Test Test");
    lines.push_back("AddTopic,Test Test");
    lines.push_back("AddTopic Test Test ");
    lines.push_back("AddTopic Test Test Test ");
    lines.push_back(";AddTopic \"Test Test\"");
    lines.push_back("AddTopic \"Test Test\" ;Test");
    vector<std::string> results;
    results.push_back("AddTopic \"Result Result\"");
    results.push_back("AddTopic \"Result Result\" ");
    results.push_back("AddTopic\"Result Result\"");
    results.push_back("AddTopic\"Result Result\" ");
    results.push_back("AddTopic,\"Result Result\"");
    results.push_back("AddTopic,\"Result Result\" ");
    results.push_back("AddTopic \"Result Result\"");
    results.push_back("AddTopic,\"Result Result\"");
    results.push_back("AddTopic \"Result Result\" ");
    results.push_back("AddTopic \"Test Test Test\" ");
    results.push_back(";AddTopic \"Test Test\"");
    results.push_back("AddTopic \"Result Result\" ;Test");

    for(size_t i = 0; i < lines.size(); ++i)
    {
        ScriptParser parser(yampt::rec_type::BNAM,
                            merger,
                            "prefix",
                            lines[i],
                            "");

        REQUIRE(parser.getNewFriendly() == results[i]);
    }
}

TEST_CASE("GetPCCell keyword")
{
    DictMerger merger;
    merger.addRecord(yampt::rec_type::CELL, "Test Test", "Result Result");
    vector<std::string> lines;
    lines.push_back("if ( GetPCCell \"Test Test\" == 1 )");
    //lines.push_back("if ( GetPCCell Test Test == 1 )");
    lines.push_back("if ( GetPCCell \"Test Test\")");
    //lines.push_back("if ( GetPCCell Test Test)");

    vector<std::string> results;
    results.push_back("if ( GetPCCell \"Result Result\" == 1 )");
    //results.push_back("if ( GetPCCell \"Result Result\" == 1 )");
    results.push_back("if ( GetPCCell \"Result Result\")");
    //results.push_back("if ( GetPCCell \"Result Result\")");

    for(size_t i = 0; i < lines.size(); ++i)
    {
        ScriptParser parser(yampt::rec_type::BNAM,
                            merger,
                            "prefix",
                            lines[i],
                            "");

        REQUIRE(parser.getNewFriendly() == results[i]);
    }
}
