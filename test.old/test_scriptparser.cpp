#include "catch.hpp"
#include "../src/config.hpp"
#include "../src/dictmerger.hpp"
#include "../src/scriptparser.hpp"

using namespace std;

TEST_CASE("Say keyword")
{
    DictMerger merger;
    merger.addRecord(yampt::rec_type::SCTX, "prefix^\"Test\"->Say \"Test.wav\" \"Test\"", "prefix^\"Test\"->Say \"Test.wav\" \"Result\"");
    vector<std::pair<std::string, std::string>> lines;
    lines.push_back(make_pair("\"Test\"->Say \"Test.wav\" \"Test\"", "\"Test\"->Say \"Test.wav\" \"Result\""));

    for(size_t i = 0; i < lines.size(); ++i)
    {
        ScriptParser parser(yampt::rec_type::SCTX,
                            merger,
                            "prefix^",
                            lines[i].first,
                            "");

        REQUIRE(parser.getNewFriendly() == lines[i].second);
    }
}

TEST_CASE("AddTopic keyword")
{
    DictMerger merger;
    merger.addRecord(yampt::rec_type::DIAL, "Test Test", "Result Result");
    vector<std::pair<std::string, std::string>> lines;
    lines.push_back(make_pair("AddTopic \"Test Test\"", "AddTopic \"Result Result\""));
    lines.push_back(make_pair("AddTopic \"Test Test\" ", "AddTopic \"Result Result\" "));
    lines.push_back(make_pair("AddTopic\"Test Test\"", "AddTopic\"Result Result\""));
    lines.push_back(make_pair("AddTopic\"Test Test\" ", "AddTopic\"Result Result\" "));
    lines.push_back(make_pair("AddTopic,\"Test Test\"", "AddTopic,\"Result Result\""));
    lines.push_back(make_pair("AddTopic,\"Test Test\" ", "AddTopic,\"Result Result\" "));
    lines.push_back(make_pair("AddTopic Test Test", "AddTopic \"Result Result\""));
    lines.push_back(make_pair("AddTopic,Test Test", "AddTopic,\"Result Result\""));
    lines.push_back(make_pair("AddTopic Test Test ", "AddTopic \"Result Result\" "));
    lines.push_back(make_pair("AddTopic Test Test Test ", "AddTopic \"Test Test Test\" "));
    lines.push_back(make_pair(";AddTopic \"Test Test\"", ";AddTopic \"Test Test\""));
    lines.push_back(make_pair("AddTopic \"Test Test\" ;Test", "AddTopic \"Result Result\" ;Test"));

    for(size_t i = 0; i < lines.size(); ++i)
    {
        ScriptParser parser(yampt::rec_type::BNAM,
                            merger,
                            "prefix",
                            lines[i].first,
                            "");

        REQUIRE(parser.getNewFriendly() == lines[i].second);
    }
}

TEST_CASE("GetPCCell keyword")
{
    DictMerger merger;
    merger.addRecord(yampt::rec_type::CELL, "Test Test", "Result Result");
    vector<std::pair<std::string, std::string>> lines;
    lines.push_back(make_pair("if ( GetPCCell \"Test Test\" == 1 )", "if ( GetPCCell \"Result Result\" == 1 )"));
    lines.push_back(make_pair("if ( GetPCCell \"Test Test\" )", "if ( GetPCCell \"Result Result\" )"));

    for(size_t i = 0; i < lines.size(); ++i)
    {
        ScriptParser parser(yampt::rec_type::BNAM,
                            merger,
                            "prefix",
                            lines[i].first,
                            "");

        REQUIRE(parser.getNewFriendly() == lines[i].second);
    }
}

TEST_CASE("GetPCCell keyword, invalid")
{
    DictMerger merger;
    merger.addRecord(yampt::rec_type::CELL, "Test Test", "Result Result");
    vector<std::pair<std::string, std::string>> lines;
    lines.push_back(make_pair("if ( GetPCCell Test Test == 1 )", "if ( GetPCCell \"Result Result\" == 1 )"));
    lines.push_back(make_pair("if ( GetPCCell Test Test )","if ( GetPCCell \"Result Result\" )"));

    for(size_t i = 0; i < lines.size(); ++i)
    {
        ScriptParser parser(yampt::rec_type::BNAM,
                            merger,
                            "prefix",
                            lines[i].first,
                            "");

        REQUIRE(parser.getNewFriendly() != lines[i].second);
    }
}
