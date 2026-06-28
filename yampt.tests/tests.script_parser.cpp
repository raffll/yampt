#include <catch2/catch_all.hpp>
#include "../yampt/utility/tools.hpp"
#include "../yampt/model/dict_merger.hpp"
#include "../yampt/model/script_parser.hpp"

using namespace std;

TEST_CASE("script_parser_t, dial keywords", "[u]")
{
	vector<std::pair<std::string, std::string>> lines {
		{ "AddTopic \"Test\"", "AddTopic \"Result\"" },
		{ "AddTopic \"Test Test\"", "AddTopic \"Result Result\"" },
		{ "AddTopic Test", "AddTopic Result" },
		{ "AddTopic Test Test", "AddTopic Result Test" },
		{ ";AddTopic \"Test\"", ";AddTopic \"Test\"" },
		{ "Player->AddTopic \"Test\"", "Player->AddTopic \"Result\"" },
		{ "AddTopic", "AddTopic" },
		{ "Begin AddTopicScript", "Begin AddTopicScript" },
		{ "AddTopicNPC->AddTopic \"Test\"", "AddTopicNPC->AddTopic \"Result\"" },
		{ "\"AddTopic NPC\"->AddTopic \"Test\"", "\"AddTopic NPC\"->AddTopic \"Test\"" }
	};

	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::dial, "Test", "Result");
	merger.add_record(tools_t::rec_type_t::dial, "Test Test", "Result Result");

	for (const auto & line : lines)
	{
		script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", line.first, "");

		REQUIRE(parser.get_new_script() == line.second);
	}
}

TEST_CASE("script_parser_t, AddTopic no-match", "[u]")
{
	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::dial, "SomeTopic", "SomeResult");

	SECTION("quoted argument not in dict is unchanged")
	{
		string input = "AddTopic \"OtherTopic\"";
		script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");
		REQUIRE(parser.get_new_script() == input);
	}

	SECTION("unquoted argument not in dict is unchanged")
	{
		string input = "AddTopic OtherTopic";
		script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");
		REQUIRE(parser.get_new_script() == input);
	}

	SECTION("comment line with AddTopic is unchanged")
	{
		string input = "; AddTopic \"SomeTopic\"";
		script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");
		REQUIRE(parser.get_new_script() == input);
	}

	SECTION("identifier containing AddTopic is unchanged")
	{
		string input = "Begin AddTopicScript";
		script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");
		REQUIRE(parser.get_new_script() == input);
	}
}

TEST_CASE("script_parser_t, cell keywords", "[u]")
{
	vector<std::pair<std::string, std::string>> lines {
		{ "if ( GetPCCell \"Test Test\" == 1 )", "if ( GetPCCell \"Result Result\" == 1 )" },
		{ "if ( GetPCCell Test == 1 )", "if ( GetPCCell Result == 1 )" },
		{ "if ( GetPCCell Test )", "if ( GetPCCell Result )" },
		{ "NPC->AiFollowCell Player \"Test\" 0 0 0 0 0", "NPC->AiFollowCell Player \"Result\" 0 0 0 0 0" },
		{ "PositionCell, 0.0, 0, 0, 0, \"Test\"", "PositionCell, 0.0, 0, 0, 0, \"Result\"" }
	};

	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::cell, "Test", "Result");
	merger.add_record(tools_t::rec_type_t::cell, "Test Test", "Result Result");

	for (const auto & line : lines)
	{
		script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", line.first, "");

		REQUIRE(parser.get_new_script() == line.second);
	}
}

TEST_CASE("script_parser_t, messages", "[u]")
{
	vector<std::pair<std::string, std::string>> lines {
		{ "NPC->Say \"Test.wav\" \"Test\"", "NPC->Say \"Test.wav\" \"Result\"" },
		{ "SayNPC->Say \"Test.wav\" \"Test\"", "SayNPC->Say \"Test.wav\" \"Result\"" },
		{ "Begin SayScript", "Begin SayScript" },
	};

	dict_merger_t merger;
	for (const auto & line : lines)
	{
		const std::string key = "^" + line.first;
		const std::string val = "^" + line.second;
		merger.add_record(tools_t::rec_type_t::bnam, key, val);
	}

	for (const auto & line : lines)
	{
		script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", line.first, "");

		REQUIRE(parser.get_new_script() == line.second);
	}
}

TEST_CASE("script_parser_t, MessageBox replacement", "[u]")
{
	const std::string script_name = "TestScript";
	const std::string input_line = "MessageBox \"Hello World\"";
	const std::string expected_line = "MessageBox \"Witaj Swiecie\"";

	const std::string key = script_name + "^" + input_line;
	const std::string value = script_name + "^" + expected_line;

	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::bnam, key, value);

	script_parser_t parser(tools_t::rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == expected_line);
}

TEST_CASE("script_parser_t, end keyword stops processing", "[u]")
{
	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::dial, "Test", "Result");

	const std::string input = "AddTopic \"Test\"\r\n"
	                          "end\r\n"
	                          "AddTopic \"Test\"";

	const std::string expected = "AddTopic \"Result\"\r\n"
	                             "end\r\n"
	                             "AddTopic \"Test\"";

	script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");
	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t, multi-line script", "[u]")
{
	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::dial, "Test", "Result");
	merger.add_record(tools_t::rec_type_t::dial, "Test Test", "Result Result");

	const std::string input = "AddTopic \"Test\"\r\n"
	                          "AddTopic \"Test Test\"";

	const std::string expected = "AddTopic \"Result\"\r\n"
	                             "AddTopic \"Result Result\"";

	script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");
	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t, trailing CRLF not appended", "[u]")
{
	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::dial, "Test", "Result");

	SECTION("input ending with CRLF preserves exactly one trailing CRLF")
	{
		const std::string input = "AddTopic \"Test\"\r\n";
		script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");
		const std::string output = parser.get_new_script();
		REQUIRE(output.size() >= 2);
		REQUIRE(output.substr(output.size() - 2) == "\r\n");
		if (output.size() >= 4)
		{
			REQUIRE(output.substr(output.size() - 4) != "\r\n\r\n");
		}
	}

	SECTION("input not ending with CRLF produces output not ending with CRLF")
	{
		const std::string input = "AddTopic \"Test\"";
		script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");
		const std::string output = parser.get_new_script();
		REQUIRE(output.size() >= 2);
		REQUIRE(output.substr(output.size() - 2) != "\r\n");
	}
}
