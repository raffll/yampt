#include <catch2/catch_all.hpp>
#include <converter/script_parser.hpp>
#include <merger/dict_merger.hpp>
#include <utility/app_logger.hpp>

static std::string size_byte(size_t value)
{
	return domain_types::convert_uint_to_string_byte_array(value).substr(0, 1);
}

TEST_CASE("script_parser_t::convert_script, scdt single replacement", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::cell, "Balmora", "Balmora PL");

	std::string scdt;
	scdt += std::string(10, '\x00');
	scdt += size_byte(7);
	scdt += "Balmora";
	scdt += std::string(5, '\x00');

	std::string input_script = "GetPCCell \"Balmora\"";

	script_parser_t parser(rec_type_t::sctx, merger, "TestScript", "test.esm", input_script, scdt);

	const auto & new_scdt = parser.get_new_scdt();
	auto found_pos = new_scdt.find("Balmora PL");
	REQUIRE(found_pos != std::string::npos);

	auto stored_size = static_cast<unsigned char>(new_scdt[found_pos - 1]);
	REQUIRE(stored_size == 10);
}

TEST_CASE("script_parser_t::convert_script, scdt multiple replacements", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Test", "Result");

	const std::string addtopic_opcode = std::string("\x22\x10", 2);

	std::string scdt;
	scdt += std::string(5, '\x00');
	scdt += addtopic_opcode;
	scdt += size_byte(4);
	scdt += "Test";
	scdt += std::string(3, '\x00');
	scdt += addtopic_opcode;
	scdt += size_byte(4);
	scdt += "Test";
	scdt += std::string(5, '\x00');

	std::string input_script = "AddTopic \"Test\"\r\nAddTopic \"Test\"";

	script_parser_t parser(rec_type_t::sctx, merger, "TestScript", "test.esm", input_script, scdt);

	const auto & new_scdt = parser.get_new_scdt();

	size_t first_pos = new_scdt.find("Result");
	REQUIRE(first_pos != std::string::npos);

	size_t second_pos = new_scdt.find("Result", first_pos + 6);
	REQUIRE(second_pos != std::string::npos);
	REQUIRE(second_pos > first_pos);

	auto first_size = static_cast<unsigned char>(new_scdt[first_pos - 1]);
	REQUIRE(first_size == 6);

	auto second_size = static_cast<unsigned char>(new_scdt[second_pos - 1]);
	REQUIRE(second_size == 6);
}

TEST_CASE("script_parser_t::convert_script, scdt getpccell size update", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::cell, "Balmora", "Bal Molagmer Hall");

	std::string old_text = "Balmora";
	std::string new_text = "Bal Molagmer Hall";

	size_t text_size = old_text.size();
	std::string comparison = " == 1";
	size_t old_expr_size = 4 + text_size + comparison.size();

	std::string scdt;
	scdt += std::string(5, '\x00');
	scdt += size_byte(old_expr_size);
	scdt += " X";
	scdt += std::string(1, '\x00');
	scdt += size_byte(text_size);
	scdt += old_text;
	scdt += comparison;
	scdt += std::string(5, '\x00');

	std::string input_script = "if ( GetPCCell \"Balmora\" == 1 )";

	script_parser_t parser(rec_type_t::sctx, merger, "TestScript", "test.esm", input_script, scdt);

	const auto & new_scdt = parser.get_new_scdt();

	auto cell_pos = new_scdt.find("Bal Molagmer Hall");
	REQUIRE(cell_pos != std::string::npos);

	auto inner_size = static_cast<unsigned char>(new_scdt[cell_pos - 1]);
	REQUIRE(inner_size == new_text.size());

	size_t x_pos = new_scdt.rfind('X', cell_pos);
	REQUIRE(x_pos != std::string::npos);

	size_t expr_size_pos = x_pos - 2;
	size_t expected_expr_size = old_expr_size + new_text.size() - old_text.size();
	auto actual_expr_size = static_cast<unsigned char>(new_scdt[expr_size_pos]);
	REQUIRE(actual_expr_size == expected_expr_size);
}

TEST_CASE("script_parser_t::convert_script, scdt length change shifts subsequent positions", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::dial, "Short", "MuchLongerName");
	merger.add_record(rec_type_t::dial, "After", "AfterResult");

	const std::string addtopic_opcode = std::string("\x22\x10", 2);

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += addtopic_opcode;
	scdt += size_byte(5);
	scdt += "Short";
	scdt += std::string(2, '\x00');
	scdt += addtopic_opcode;
	scdt += size_byte(5);
	scdt += "After";
	scdt += std::string(3, '\x00');

	std::string input_script = "AddTopic \"Short\"\r\nAddTopic \"After\"";

	script_parser_t parser(rec_type_t::sctx, merger, "TestScript", "test.esm", input_script, scdt);

	const auto & new_scdt = parser.get_new_scdt();

	auto first_pos = new_scdt.find("MuchLongerName");
	REQUIRE(first_pos != std::string::npos);

	auto second_pos = new_scdt.find("AfterResult");
	REQUIRE(second_pos != std::string::npos);
	REQUIRE(second_pos > first_pos + 14);

	auto second_size = static_cast<unsigned char>(new_scdt[second_pos - 1]);
	REQUIRE(second_size == 11);
}

TEST_CASE("script_parser_t::convert_script, scdt message replacement with size prefix", "[u]")
{
	const std::string script_name = "TestScript";
	const std::string input_line = "MessageBox \"Hello World\"";
	const std::string output_line = "MessageBox \"Witaj Swiecie\"";

	dict_merger_t merger;
	merger.add_record(rec_type_t::sctx, script_name + "^" + input_line, output_line);

	std::string old_msg = "Hello World";
	std::string new_msg = "Witaj Swiecie";

	std::string scdt;
	scdt += std::string(4, '\x00');
	scdt += domain_types::convert_uint_to_string_byte_array(old_msg.size()).substr(0, 2);
	scdt += old_msg;
	scdt += std::string(5, '\x00');

	script_parser_t parser(rec_type_t::sctx, merger, script_name, "test.esm", input_line, scdt);

	const auto & new_scdt = parser.get_new_scdt();

	auto msg_pos = new_scdt.find("Witaj Swiecie");
	REQUIRE(msg_pos != std::string::npos);

	auto stored_size_lo = static_cast<unsigned char>(new_scdt[msg_pos - 2]);
	auto stored_size_hi = static_cast<unsigned char>(new_scdt[msg_pos - 1]);
	size_t stored_size = stored_size_lo | (stored_size_hi << 8);
	REQUIRE(stored_size == new_msg.size());
}

TEST_CASE("script_parser_t::convert_script, scdt choice blob includes trailing number", "[u]")
{
	const std::string script_name = "TestScript";
	const std::string input_line = "choice \"Yes here.\" 11 \"Nevermind.\" 10";
	const std::string output_line = "choice \"Tak tutaj.\" 11 \"Niewazne.\" 10";

	dict_merger_t merger;
	merger.add_record(rec_type_t::sctx, script_name + "^" + input_line, output_line);

	const std::string old_blob = "\"Yes here.\" 11 \"Nevermind.\" 10";
	const std::string new_blob = "\"Tak tutaj.\" 11 \"Niewazne.\" 10";

	std::string scdt;
	scdt += std::string(2, '\x00');
	scdt += std::string(1, '\xc9');
	scdt += std::string(1, '\x10');
	scdt += domain_types::convert_uint_to_string_byte_array(old_blob.size()).substr(0, 2);
	scdt += old_blob;
	scdt += std::string(4, '\x00');

	script_parser_t parser(rec_type_t::sctx, merger, script_name, "test.esm", input_line, scdt);

	const auto & new_scdt = parser.get_new_scdt();

	const auto blob_pos = new_scdt.find(new_blob);
	REQUIRE(blob_pos != std::string::npos);
	REQUIRE(new_scdt.find(old_blob) == std::string::npos);

	const auto stored_size_lo = static_cast<unsigned char>(new_scdt[blob_pos - 2]);
	const auto stored_size_hi = static_cast<unsigned char>(new_scdt[blob_pos - 1]);
	const size_t stored_size = stored_size_lo | (stored_size_hi << 8);
	REQUIRE(stored_size == new_blob.size());
}

TEST_CASE("script_parser_t::convert_script, messagebox buttons patched as null-terminated segments", "[u]")
{
	const std::string script_name = "TestScript";
	const std::string input_line = "MessageBox \"Return home?\" \"Yes\" \"No\"";
	const std::string output_line = "MessageBox \"Wrocic?\" \"Tak\" \"Nie\"";

	dict_merger_t merger;
	merger.add_record(rec_type_t::sctx, script_name + "^" + input_line, output_line);

	const std::string message = "Return home?";
	const std::string btn_yes = "Yes";
	const std::string btn_no = "No";

	std::string scdt;
	scdt += std::string(1, '\x10');
	scdt += domain_types::convert_uint_to_string_byte_array(message.size()).substr(0, 2);
	scdt += message;
	scdt += std::string(1, '\x00');
	scdt += std::string(1, '\x02');
	scdt += size_byte(btn_yes.size() + 1);
	scdt += btn_yes;
	scdt += std::string(1, '\x00');
	scdt += size_byte(btn_no.size() + 1);
	scdt += btn_no;
	scdt += std::string(1, '\x00');
	scdt += std::string(3, '\x00');

	script_parser_t parser(rec_type_t::sctx, merger, script_name, "test.esm", input_line, scdt);

	const auto & new_scdt = parser.get_new_scdt();

	REQUIRE(new_scdt.find("Wrocic?") != std::string::npos);

	const auto tak_pos = new_scdt.find("Tak");
	REQUIRE(tak_pos != std::string::npos);
	REQUIRE(static_cast<unsigned char>(new_scdt[tak_pos - 1]) == std::string("Tak").size() + 1);
	REQUIRE(new_scdt[tak_pos + 3] == '\x00');

	const auto nie_pos = new_scdt.find("Nie");
	REQUIRE(nie_pos != std::string::npos);
	REQUIRE(static_cast<unsigned char>(new_scdt[nie_pos - 1]) == std::string("Nie").size() + 1);
	REQUIRE(new_scdt[nie_pos + 3] == '\x00');
}

TEST_CASE("script_parser_t::convert_script, scdt say patches subtitle keeps sound file", "[u]")
{
	const std::string script_name = "TestScript";
	const std::string input_line = "Say \"Vo\\atk.mp3\" \"You will pay!\"";
	const std::string output_line = "Say \"Vo\\atk.mp3\" \"Zaplacisz!\"";

	dict_merger_t merger;
	merger.add_record(rec_type_t::sctx, script_name + "^" + input_line, output_line);

	const std::string sound = "Vo\\atk.mp3";
	const std::string subtitle = "You will pay!";

	std::string scdt;
	scdt += std::string(1, '\x1b');
	scdt += std::string(1, '\x11');
	scdt += size_byte(sound.size());
	scdt += sound;
	scdt += std::string(1, '\x10');
	scdt += domain_types::convert_uint_to_string_byte_array(subtitle.size()).substr(0, 2);
	scdt += subtitle;
	scdt += std::string(3, '\x00');

	script_parser_t parser(rec_type_t::sctx, merger, script_name, "test.esm", input_line, scdt);

	const auto & new_scdt = parser.get_new_scdt();

	REQUIRE(new_scdt.find("Zaplacisz!") != std::string::npos);
	REQUIRE(new_scdt.find("You will pay!") == std::string::npos);
	REQUIRE(new_scdt.find(sound) != std::string::npos);
}

TEST_CASE("script_parser_t::convert_script, scdt messagebox keeps vertical bar literal", "[u]")
{
	const std::string script_name = "TestScript";
	const std::string input_line = "MessageBox \"Shipyard | 3E 371\"";
	const std::string output_line = "MessageBox \"Stocznia | Cesarstwo 3E 371\"";

	dict_merger_t merger;
	merger.add_record(rec_type_t::sctx, script_name + "^" + input_line, output_line);

	const std::string message = "Shipyard | 3E 371";

	std::string scdt;
	scdt += std::string(1, '\x10');
	scdt += domain_types::convert_uint_to_string_byte_array(message.size()).substr(0, 2);
	scdt += message;
	scdt += std::string(3, '\x00');

	script_parser_t parser(rec_type_t::sctx, merger, script_name, "test.esm", input_line, scdt);

	const auto & new_scdt = parser.get_new_scdt();

	REQUIRE(new_scdt.find("Stocznia | Cesarstwo 3E 371") != std::string::npos);
	REQUIRE(new_scdt.find('|') != std::string::npos);
	REQUIRE(new_scdt.find('\x0a') == std::string::npos);
}
