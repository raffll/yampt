#include "catch.hpp"
#include "../yampt/config.hpp"
#include "../yampt/dictmerger.hpp"
#include "../yampt/scriptparser.hpp"

using namespace std;

struct Item
{
    std::string input;
    std::string expected;
};

TEST_CASE("Say keyword")
{
    std::vector<Item> items;
    items.push_back({ R"("Test"->Say "Test.wav" "Input")", R"("Test"->Say "Test.wav" "Expected")" });
    items.push_back({ R"(Test->Say "Test.wav" "Input")", R"(Test->Say "Test.wav" "Expected")" });
    items.push_back({ R"("Choice"->Say "Test.wav" "Input")", R"("Choice"->Say "Test.wav" "Expected")" });
    items.push_back({ R"(Choice->Say "Test.wav" "Input")", R"(Choice->Say "Test.wav" "Expected")" });

    DictMerger merger;
    for (const auto & item : items)
        merger.addRecord(yampt::rec_type::SCTX, item.input, item.expected);

    for (const auto & item : items)
    {
        ScriptParser parser(
            yampt::rec_type::SCTX,
            merger,
            "",
            item.input,
            "");

        REQUIRE(parser.getNewFriendly() == item.expected);
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

    for (size_t i = 0; i < lines.size(); ++i)
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

    for (size_t i = 0; i < lines.size(); ++i)
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
    lines.push_back(make_pair("if ( GetPCCell Test Test )", "if ( GetPCCell \"Result Result\" )"));

    for (size_t i = 0; i < lines.size(); ++i)
    {
        ScriptParser parser(yampt::rec_type::BNAM,
            merger,
            "prefix",
            lines[i].first,
            "");

        REQUIRE(parser.getNewFriendly() != lines[i].second);
    }
}
