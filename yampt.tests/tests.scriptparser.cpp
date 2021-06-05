#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dictmerger.hpp"
#include "../yampt/scriptparser.hpp"

using namespace std;

TEST_CASE("script parser, dial keywords", "[u]")
{
    vector<std::pair<std::string, std::string>> lines
    {
        { R"(AddTopic "Test")", R"(AddTopic "Result")" },
        { R"(AddTopic "Test Test")", R"(AddTopic "Result Result")" },
        { R"(AddTopic Test)", R"(AddTopic Result)" },
        { R"(AddTopic Test Test)", R"(AddTopic Result Test)" }, /* need to check if possible */
        { R"(;AddTopic "Test")", R"(;AddTopic "Test")" },
        { R"(Player->AddTopic "Test")", R"(Player->AddTopic "Result")" },
        { R"(AddTopic)", R"(AddTopic)" },
        { R"(Begin AddTopicScript)", R"(Begin AddTopicScript)" },
        { R"(AddTopicNPC->AddTopic "Test")", R"(AddTopicNPC->AddTopic "Result")" },
        { R"("AddTopic NPC"->AddTopic "Test")", R"("AddTopic NPC"->AddTopic "Result")" }
    };

    DictMerger merger;
    merger.addRecord(Tools::RecType::DIAL, "Test", "Result");
    merger.addRecord(Tools::RecType::DIAL, "Test Test", "Result Result");

    for (const auto & line : lines)
    {
        ScriptParser parser(
            Tools::RecType::BNAM,
            merger,
            "",
            "",
            line.first,
            "");

        REQUIRE(parser.getNewScript() == line.second);
    }
}

TEST_CASE("script parser, cell keywords", "[u]")
{
    vector<std::pair<std::string, std::string>> lines
    {
        { R"(if ( GetPCCell "Test Test" == 1 ))", R"(if ( GetPCCell "Result Result" == 1 ))" },
        { R"(if ( GetPCCell Test == 1 ))", R"(if ( GetPCCell Result == 1 ))" },
        { R"(if ( GetPCCell Test ))", R"(if ( GetPCCell Result ))" },
        { R"(NPC->AiFollowCell Player "Test" 0 0 0 0 0)", R"(NPC->AiFollowCell Player "Result" 0 0 0 0 0)" },
        { R"(PositionCell, 0.0, 0, 0, 0, "Test")", R"(PositionCell, 0.0, 0, 0, 0, "Result")" }
    };

    DictMerger merger;
    merger.addRecord(Tools::RecType::CELL, "Test", "Result");
    merger.addRecord(Tools::RecType::CELL, "Test Test", "Result Result");

    for (const auto & line : lines)
    {
        ScriptParser parser(
            Tools::RecType::BNAM,
            merger,
            "",
            "",
            line.first,
            "");

        REQUIRE(parser.getNewScript() == line.second);
    }
}

TEST_CASE("script parser, messages", "[u]")
{
    vector<std::pair<std::string, std::string>> lines
    {
        { R"(NPC->Say "Test.wav" "Test")", R"(NPC->Say "Test.wav" "Result")" },
        { R"(SayNPC->Say "Test.wav" "Test")", R"(SayNPC->Say "Test.wav" "Result")" },
        { R"(Begin SayScript)", R"(Begin SayScript)" },
    };

    DictMerger merger;
    for (const auto & line : lines)
        merger.addRecord(Tools::RecType::BNAM, line.first, line.second);

    for (const auto & line : lines)
    {
        ScriptParser parser(
            Tools::RecType::BNAM,
            merger,
            "",
            "",
            line.first,
            "");

        REQUIRE(parser.getNewScript() == line.second);
    }
}
