#include <catch2/catch_all.hpp>
#include <converter/script_parser.hpp>
#include <merger/dict_merger.hpp>
#include <utility/app_logger.hpp>

TEST_CASE("script_parser_t::convert_script, record_key used for message lookup", "[u]")
{
	const std::string record_key = "T^some topic^12345";
	const std::string input_line = "MessageBox \"Original\"";
	const std::string translated_line = "MessageBox \"Przetlumaczony\"";

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, record_key + "^" + input_line, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, record_key, "test.esm", input_line, "");

	REQUIRE(parser.get_new_script() == translated_line);
}

TEST_CASE("script_parser_t::convert_script, empty record_key still finds messages", "[u]")
{
	const std::string input_line = "MessageBox \"Original\"";
	const std::string translated_line = "MessageBox \"Przetlumaczony\"";

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, "^" + input_line, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, "", "test.esm", input_line, "");

	REQUIRE(parser.get_new_script() == translated_line);
}

TEST_CASE("script_parser_t::convert_script, wrong record_key prevents match", "[u]")
{
	const std::string correct_key = "T^topic^111";
	const std::string wrong_key = "T^topic^222";
	const std::string input_line = "MessageBox \"Hello\"";
	const std::string translated_line = "MessageBox \"Witaj\"";

	dict_merger_t merger;
	merger.add_record(rec_type_t::bnam, correct_key + "^" + input_line, translated_line);

	script_parser_t parser(rec_type_t::bnam, merger, wrong_key, "test.esm", input_line, "");

	REQUIRE(parser.get_new_script() == input_line);
}

TEST_CASE("script_parser_t::convert_script, source_path does not affect conversion", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Test", "Result");

	const std::string input = "AddTopic \"Test\"";
	const std::string expected = "AddTopic \"Result\"";

	script_parser_t parser_with_path(rec_type_t::bnam, merger, "", "C:/long/path/to/file.esm", input, "");
	REQUIRE(parser_with_path.get_new_script() == expected);

	script_parser_t parser_empty_path(rec_type_t::bnam, merger, "", "", input, "");
	REQUIRE(parser_empty_path.get_new_script() == expected);
}

TEST_CASE("script_parser_t::convert_script, empty input produces empty output", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Test", "Result");

	script_parser_t parser(rec_type_t::bnam, merger, "", "", "", "");

	REQUIRE(parser.get_new_script().empty());
}

TEST_CASE("script_parser_t::convert_script, single line input without CRLF", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Test", "Result");

	const std::string input = "AddTopic \"Test\"";
	const std::string expected = "AddTopic \"Result\"";

	script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t::convert_script, input with no translatable content unchanged", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Test", "Result");

	const std::string input = "set x to 1\r\nset y to 2\r\nif (x > y)\r\nendif";

	script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == input);
}

TEST_CASE("script_parser_t::convert_script, empty merger leaves everything unchanged", "[u]")
{
	dict_merger_t merger;

	const std::string input = "AddTopic \"SomeTopic\"\r\nGetPCCell \"SomeCell\"";

	script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == input);
}

TEST_CASE("script_parser_t::convert_script, sctx type with empty scdt skips compiled patching", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Hello", "Witaj");

	const std::string input = "AddTopic \"Hello\"";
	const std::string expected = "AddTopic \"Witaj\"";

	script_parser_t parser(rec_type_t::sctx, merger, "TestScript", "test.esm", input, "");

	REQUIRE(parser.get_new_script() == expected);
	REQUIRE(parser.get_new_scdt().empty());
}

TEST_CASE("script_parser_t::convert_script, multiple keywords on separate lines", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "TopicA", "TematA");
	merger.add_record(rec_type_t::cell, "CellB", "KomorkaB");

	const std::string input = "AddTopic \"TopicA\"\r\nGetPCCell \"CellB\"";
	const std::string expected = "AddTopic \"TematA\"\r\nGetPCCell \"KomorkaB\"";

	script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t::convert_script, line after end keyword is not processed", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Before", "Przed");
	merger.add_record(rec_type_t::dial, "After", "Po");

	const std::string input = "AddTopic \"Before\"\r\nend MyScript\r\nAddTopic \"After\"";
	const std::string expected = "AddTopic \"Przed\"\r\nend MyScript\r\nAddTopic \"After\"";

	script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t::convert_script, case insensitive cell lookup", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::cell, "balmora", "Balmora PL");

	const std::string input = "GetPCCell \"Balmora\"";
	const std::string expected = "GetPCCell \"Balmora PL\"";

	script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t::convert_script, case insensitive dial lookup", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "background", "tlo");

	const std::string input = "AddTopic \"Background\"";
	const std::string expected = "AddTopic \"tlo\"";

	script_parser_t parser(rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == expected);
}
