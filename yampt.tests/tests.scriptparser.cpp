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
        { R"("AddTopic NPC"->AddTopic "Test")", R"("AddTopic NPC"->AddTopic "Test")" }
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

TEST_CASE("script parser, AddTopic no-match", "[u]")
{
    DictMerger merger;
    merger.addRecord(Tools::RecType::DIAL, "SomeTopic", "SomeResult");

    SECTION("quoted argument not in dict is unchanged")
    {
        string input = R"(AddTopic "OtherTopic")";
        ScriptParser parser(Tools::RecType::BNAM, merger, "", "", input, "");
        REQUIRE(parser.getNewScript() == input);
    }

    SECTION("unquoted argument not in dict is unchanged")
    {
        string input = R"(AddTopic OtherTopic)";
        ScriptParser parser(Tools::RecType::BNAM, merger, "", "", input, "");
        REQUIRE(parser.getNewScript() == input);
    }

    SECTION("comment line with AddTopic is unchanged")
    {
        string input = R"(; AddTopic "SomeTopic")";
        ScriptParser parser(Tools::RecType::BNAM, merger, "", "", input, "");
        REQUIRE(parser.getNewScript() == input);
    }

    SECTION("identifier containing AddTopic is unchanged")
    {
        string input = R"(Begin AddTopicScript)";
        ScriptParser parser(Tools::RecType::BNAM, merger, "", "", input, "");
        REQUIRE(parser.getNewScript() == input);
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
    {
        const std::string key = Tools::sep[0] + line.first;
        const std::string val = Tools::sep[0] + line.second;
        merger.addRecord(Tools::RecType::BNAM, key, val);
    }

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

TEST_CASE("script parser, MessageBox replacement", "[u]")
{
    // **Validates: Requirements 11.2**
    const std::string script_name = "TestScript";
    const std::string input_line = R"(MessageBox "Hello World")";
    const std::string expected_line = R"(MessageBox "Witaj Swiecie")";

    const std::string key = script_name + Tools::sep[0] + input_line;
    const std::string value = script_name + Tools::sep[0] + expected_line;

    DictMerger merger;
    merger.addRecord(Tools::RecType::BNAM, key, value);

    ScriptParser parser(
        Tools::RecType::BNAM,
        merger,
        script_name,
        "",
        input_line,
        "");

    REQUIRE(parser.getNewScript() == expected_line);
}

TEST_CASE("script parser, end keyword stops processing", "[u]")
{
    // Validates: Requirements 10.1, 12.1
    // Lines after "end" must not be modified
    DictMerger merger;
    merger.addRecord(Tools::RecType::DIAL, "Test", "Result");

    const std::string input =
        R"(AddTopic "Test")" "\r\n"
        "end\r\n"
        R"(AddTopic "Test")";

    const std::string expected =
        R"(AddTopic "Result")" "\r\n"
        "end\r\n"
        R"(AddTopic "Test")";

    ScriptParser parser(Tools::RecType::BNAM, merger, "", "", input, "");
    REQUIRE(parser.getNewScript() == expected);
}

TEST_CASE("script parser, multi-line script", "[u]")
{
    // Validates: Requirements 11.4, 12.1
    // Each line is processed independently; full output is correct
    DictMerger merger;
    merger.addRecord(Tools::RecType::DIAL, "Test", "Result");
    merger.addRecord(Tools::RecType::DIAL, "Test Test", "Result Result");

    const std::string input =
        R"(AddTopic "Test")" "\r\n"
        R"(AddTopic "Test Test")";

    const std::string expected =
        R"(AddTopic "Result")" "\r\n"
        R"(AddTopic "Result Result")";

    ScriptParser parser(Tools::RecType::BNAM, merger, "", "", input, "");
    REQUIRE(parser.getNewScript() == expected);
}

TEST_CASE("script parser, trailing CRLF not appended", "[u]")
{
    // Validates: Requirements 12.2, 12.3
    DictMerger merger;
    merger.addRecord(Tools::RecType::DIAL, "Test", "Result");

    SECTION("input ending with CRLF preserves exactly one trailing CRLF")
    {
        const std::string input = R"(AddTopic "Test")" "\r\n";
        ScriptParser parser(Tools::RecType::BNAM, merger, "", "", input, "");
        const std::string output = parser.getNewScript();
        REQUIRE(output.size() >= 2);
        REQUIRE(output.substr(output.size() - 2) == "\r\n");
        // No extra \r\n appended beyond the one already present
        if (output.size() >= 4)
        {
            REQUIRE(output.substr(output.size() - 4) != "\r\n\r\n");
        }
    }

    SECTION("input not ending with CRLF produces output not ending with CRLF")
    {
        const std::string input = R"(AddTopic "Test")";
        ScriptParser parser(Tools::RecType::BNAM, merger, "", "", input, "");
        const std::string output = parser.getNewScript();
        REQUIRE(output.size() >= 2);
        REQUIRE(output.substr(output.size() - 2) != "\r\n");
    }
}
