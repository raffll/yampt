#include <catch2/catch_all.hpp>
#include <converter/script_parser.hpp>
#include <merger/dict_merger.hpp>
#include <utility/app_logger.hpp>

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
	merger.add_record(rec_type_t::dial, "Test", "Result");
	merger.add_record(rec_type_t::dial, "Test Test", "Result Result");

	for (const auto & line : lines)
	{
		script_parser_t parser(rec_type_t::bnam, merger, "", "", line.first, "");

		REQUIRE(parser.get_new_script() == line.second);
	}
}

TEST_CASE("script_parser_t, AddTopic no-match", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "SomeTopic", "SomeResult");

	SECTION("quoted argument not in dict is unchanged")
	{
		string input = "AddTopic \"OtherTopic\"";
		script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");
		REQUIRE(parser.get_new_script() == input);
	}

	SECTION("unquoted argument not in dict is unchanged")
	{
		string input = "AddTopic OtherTopic";
		script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");
		REQUIRE(parser.get_new_script() == input);
	}

	SECTION("comment line with AddTopic is unchanged")
	{
		string input = "; AddTopic \"SomeTopic\"";
		script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");
		REQUIRE(parser.get_new_script() == input);
	}

	SECTION("identifier containing AddTopic is unchanged")
	{
		string input = "Begin AddTopicScript";
		script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");
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
	merger.add_record(rec_type_t::cell, "Test", "Result");
	merger.add_record(rec_type_t::cell, "Test Test", "Result Result");

	for (const auto & line : lines)
	{
		script_parser_t parser(rec_type_t::bnam, merger, "", "", line.first, "");

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
		const std::string val = line.second;
		merger.add_record(rec_type_t::bnam, key, val);
	}

	for (const auto & line : lines)
	{
		script_parser_t parser(rec_type_t::bnam, merger, "", "", line.first, "");

		REQUIRE(parser.get_new_script() == line.second);
	}
}

TEST_CASE("script_parser_t, MessageBox replacement", "[u]")
{
	const std::string script_name = "TestScript";
	const std::string input_line = "MessageBox \"Hello World\"";
	const std::string expected_line = "MessageBox \"Witaj Swiecie\"";

	const std::string key = script_name + "^" + input_line;
	const std::string value = expected_line;

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, key, value);

	script_parser_t parser(rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == expected_line);
}

TEST_CASE("script_parser_t, end keyword stops processing", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Test", "Result");

	const std::string input = "AddTopic \"Test\"\r\n"
	                          "end\r\n"
	                          "AddTopic \"Test\"";

	const std::string expected = "AddTopic \"Result\"\r\n"
	                             "end\r\n"
	                             "AddTopic \"Test\"";

	script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");
	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t, multi-line script", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Test", "Result");
	merger.add_record(rec_type_t::dial, "Test Test", "Result Result");

	const std::string input = "AddTopic \"Test\"\r\n"
	                          "AddTopic \"Test Test\"";

	const std::string expected = "AddTopic \"Result\"\r\n"
	                             "AddTopic \"Result Result\"";

	script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");
	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t, trailing CRLF not appended", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Test", "Result");

	SECTION("input ending with CRLF preserves exactly one trailing CRLF")
	{
		const std::string input = "AddTopic \"Test\"\r\n";
		script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");
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
		script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");
		const std::string output = parser.get_new_script();
		REQUIRE(output.size() >= 2);
		REQUIRE(output.substr(output.size() - 2) != "\r\n");
	}
}

TEST_CASE("script_parser_t::find_new_message, short new_text", "[u]")
{
	const std::string script_name = "TR_m3_OE_TG_ChunzefkRockSCP";
	const std::string input_line = "         MessageBox \"    5    \"";
	const std::string translated_line = "         MessageBox \"    5    \"";

	const std::string key = script_name + "^" + input_line;

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, key, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == translated_line);
}

TEST_CASE("script_parser_t::find_new_message, applies translation", "[u]")
{
	const std::string script_name = "TR_m4_VM_Headless_ReqsCheckSc";
	const std::string input_line = "Choice \"Continue\" 2";
	const std::string translated_line = "Choice \"Dalej\" 2";

	const std::string key = script_name + "^" + input_line;

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, key, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == translated_line);
}

TEST_CASE("script_parser_t::find_new_message, long name short text", "[u]")
{
	const std::string script_name = "TR_m3_act_OE_LegHQnails_scpt";
	const std::string input_line = "         MessageBox, \"You are here.\"";
	const std::string translated_line = "         MessageBox, \"Jestes tutaj\"";

	const std::string key = script_name + "^" + input_line;

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, key, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == translated_line);
}

TEST_CASE("script_parser_t::find_new_message, no match unchanged", "[u]")
{
	const std::string script_name = "SomeScript";
	const std::string input_line = "MessageBox \"Unregistered text\"";

	dict_merger_t merger;

	script_parser_t parser(rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == input_line);
}

TEST_CASE("script_parser_t::find_new_message, identical skipped", "[u]")
{
	const std::string script_name = "TestScript";
	const std::string input_line = "MessageBox \"Same text\"";

	const std::string key = script_name + "^" + input_line;

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, key, input_line);

	script_parser_t parser(rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == input_line);
}

TEST_CASE("script_parser_t::find_new_message, multi-quoted", "[u]")
{
	const std::string script_name = "TestScript";
	const std::string input_line = "MessageBox \"Move shield?\" \"Yes\" \"No\"";
	const std::string translated_line = "MessageBox \"Przesunac?\" \"Tak\" \"Nie\"";

	const std::string key = script_name + "^" + input_line;

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, key, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == translated_line);
}

TEST_CASE("script_parser_t::find_new_message, choice with number", "[u]")
{
	const std::string script_name = "QuestScript";
	const std::string input_line = "choice \"Nevermind.\" 2";
	const std::string translated_line = "choice \"Zapomnij.\" 2";

	const std::string key = script_name + "^" + input_line;

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, key, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == translated_line);
}

TEST_CASE("script_parser_t::find_new_message, longer translation", "[u]")
{
	const std::string script_name = "Sc";
	const std::string input_line = "MessageBox \"Hi\"";
	const std::string translated_line = "MessageBox \"Bardzo dluga przetlumaczona wiadomosc\"";

	const std::string key = script_name + "^" + input_line;

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, key, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == translated_line);
}

TEST_CASE("script_parser_t, bnam choice with composite key", "[u]")
{
	const std::string dial_type = "T";
	const std::string dial_name = "some topic";
	const std::string inam = "2483419585499221";
	const std::string script_name = dial_type + "^" + dial_name + "^" + inam;

	const std::string input_line = "choice \"Go away\" 1 \"Tell me more\" 2";
	const std::string translated_line = "choice \"Odejdz\" 1 \"Powiedz wiecej\" 2";

	const std::string key = script_name + "^" + input_line;

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, key, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, script_name, "", input_line, "");

	REQUIRE(parser.get_new_script() == translated_line);
}

TEST_CASE("script_parser_t, bnam choice not found with wrong key", "[u]")
{
	const std::string inam = "2483419585499221";
	const std::string full_key_prefix = "T^some topic^" + inam;
	const std::string wrong_prefix = inam;

	const std::string input_line = "choice \"Go away\" 1 \"Tell me more\" 2";
	const std::string translated_line = "choice \"Odejdz\" 1 \"Powiedz wiecej\" 2";

	const std::string key = full_key_prefix + "^" + input_line;

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, key, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, wrong_prefix, "", input_line, "");

	REQUIRE(parser.get_new_script() == input_line);
}
