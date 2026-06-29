#include <catch2/catch_all.hpp>
#include <utility/tools.hpp>

TEST_CASE("tools_t::convert_string_byte_array_to_uint, basic conversions", "[u]")
{
	std::string text = "DEAD";
	REQUIRE(tools_t::convert_string_byte_array_to_uint(text) == 1145128260);
	text = "D";
	REQUIRE(tools_t::convert_string_byte_array_to_uint(text) == 68);
	for (int i = 0; i < 3; ++i)
	{
		text += '\0';
	}
	REQUIRE(text.size() == 4);
	REQUIRE(tools_t::convert_string_byte_array_to_uint(text) == 68);
}

TEST_CASE("tools_t::convert_uint_to_string_byte_array, basic conversion", "[u]")
{
	REQUIRE(tools_t::convert_uint_to_string_byte_array(1145128260) == "DEAD");
}

TEST_CASE("tools_t::case_insensitive_string_cmp, matches and mismatches", "[u]")
{
	REQUIRE(tools_t::case_insensitive_string_cmp("DEAD", "dead") == true);
	REQUIRE(tools_t::case_insensitive_string_cmp("DEAD", "BEEF") == false);
}

TEST_CASE("tools_t::erase_null_chars, truncates at first null", "[u]")
{
	std::string text = "DEAD";
	text.resize(8);
	REQUIRE(tools_t::erase_null_chars(text) == "DEAD");
	text = "DEAD";
	text.resize(8);
	text += "BEEF";
	REQUIRE(tools_t::erase_null_chars(text) == "DEAD");
}

TEST_CASE("tools_t::trim_cr, removes only trailing CR", "[u]")
{
	std::string text = "DEAD\r";
	REQUIRE(tools_t::trim_cr(text) == "DEAD");
	text = "DE\rAD\r";
	REQUIRE(tools_t::trim_cr(text) == "DE\rAD");
}

TEST_CASE("tools_t::chapter_t::insert, new and duplicate keys", "[u]")
{
	tools_t::chapter_t chapter;

	SECTION("inserting a new entry returns true")
	{
		bool result = chapter.insert({ "key1", "orig1", "trans1", status_t::untranslated });
		REQUIRE(result == true);
	}

	SECTION("inserting duplicate key returns false")
	{
		chapter.insert({ "key1", "orig1", "trans1", status_t::untranslated });
		bool result = chapter.insert({ "key1", "orig2", "trans2", status_t::translated });
		REQUIRE(result == false);
	}
}

TEST_CASE("tools_t::chapter_t::find, existing and missing keys", "[u]")
{
	tools_t::chapter_t chapter;
	chapter.insert({ "key1", "orig1", "trans1", status_t::untranslated });

	SECTION("finding existing id returns non-null pointer with correct data")
	{
		auto * entry = chapter.find("key1");
		REQUIRE(entry != nullptr);
		REQUIRE(entry->key_text == "key1");
		REQUIRE(entry->old_text == "orig1");
		REQUIRE(entry->new_text == "trans1");
		REQUIRE(entry->status == status_t::untranslated);
	}

	SECTION("finding non-existent id returns nullptr")
	{
		auto * entry = chapter.find("no_such_key");
		REQUIRE(entry == nullptr);
	}
}

TEST_CASE("tools_t::chapter_t::size, counts unique entries", "[u]")
{
	tools_t::chapter_t chapter;
	REQUIRE(chapter.size() == 0);

	chapter.insert({ "key1", "orig1", "trans1", status_t::untranslated });
	REQUIRE(chapter.size() == 1);

	chapter.insert({ "key2", "orig2", "trans2", status_t::translated });
	REQUIRE(chapter.size() == 2);

	chapter.insert({ "key1", "dup", "dup", status_t::duplicate });
	REQUIRE(chapter.size() == 2);
}

TEST_CASE("tools_t::chapter_t::empty, reflects entry count", "[u]")
{
	tools_t::chapter_t chapter;
	REQUIRE(chapter.empty() == true);

	chapter.insert({ "key1", "orig1", "trans1", status_t::untranslated });
	REQUIRE(chapter.empty() == false);
}

TEST_CASE("tools_t::type_to_str and str_to_type, round-trip", "[u]")
{
	const std::vector<tools_t::rec_type_t> defined_types {
		tools_t::rec_type_t::cell, tools_t::rec_type_t::dial, tools_t::rec_type_t::indx, tools_t::rec_type_t::rnam,
		tools_t::rec_type_t::desc, tools_t::rec_type_t::gmst, tools_t::rec_type_t::fnam, tools_t::rec_type_t::info,
		tools_t::rec_type_t::text, tools_t::rec_type_t::bnam, tools_t::rec_type_t::sctx,
	};

	for (const auto & type : defined_types)
	{
		REQUIRE(tools_t::str_to_type(tools_t::type_to_str(type)) == type);
	}

	SECTION("str_to_type returns Unknown for unrecognized string")
	{
		REQUIRE(tools_t::str_to_type("XYZZY") == tools_t::rec_type_t::unknown);
		REQUIRE(tools_t::str_to_type("") == tools_t::rec_type_t::unknown);
		REQUIRE(tools_t::str_to_type("cell") == tools_t::rec_type_t::unknown);
	}
}

TEST_CASE("tools_t::get_dialog_type, all values", "[u]")
{
	REQUIRE(tools_t::get_dialog_type(std::string(1, '\x00')) == "T");
	REQUIRE(tools_t::get_dialog_type(std::string(1, '\x01')) == "V");
	REQUIRE(tools_t::get_dialog_type(std::string(1, '\x02')) == "G");
	REQUIRE(tools_t::get_dialog_type(std::string(1, '\x03')) == "P");
	REQUIRE(tools_t::get_dialog_type(std::string(1, '\x04')) == "J");
}

TEST_CASE("tools_t::get_indx, zero-padded output", "[u]")
{
	// 4-byte little-endian encoding of 1 → "001"
	std::string one = tools_t::convert_uint_to_string_byte_array(1);
	REQUIRE(tools_t::get_indx(one) == "001");

	// 4-byte little-endian encoding of 42 → "042"
	std::string fortytwo = tools_t::convert_uint_to_string_byte_array(42);
	REQUIRE(tools_t::get_indx(fortytwo) == "042");

	// 4-byte little-endian encoding of 255 → "255"
	std::string twofiftyfive = tools_t::convert_uint_to_string_byte_array(255);
	REQUIRE(tools_t::get_indx(twofiftyfive) == "255");
}

TEST_CASE("tools_t::is_fnam, true IDs", "[u]")
{
	const std::vector<std::string> true_ids {
		"ACTI", "ALCH", "APPA", "ARMO", "BOOK", "BSGN", "CLAS", "CLOT", "CONT", "CREA", "DOOR", "FACT",
		"INGR", "LIGH", "LOCK", "MISC", "NPC_", "PROB", "RACE", "REGN", "REPA", "SPEL", "WEAP",
	};

	for (const auto & id : true_ids)
	{
		REQUIRE(tools_t::is_fnam(id) == true);
	}
}

TEST_CASE("tools_t::is_fnam, false IDs", "[u]")
{
	REQUIRE(tools_t::is_fnam("CELL") == false);
	REQUIRE(tools_t::is_fnam("INFO") == false);
	REQUIRE(tools_t::is_fnam("DIAL") == false);
}

TEST_CASE("tools_t::byte conversion, round-trip", "[u]")
{
	const unsigned int values[] = {
		0u,          1u, 127u, 128u, 255u, 256u, 65535u, 65536u, 0x7FFFFFFFu, 0xFFFFFFFFu,
		0x01020304u, // all four bytes non-zero
		0x0A0B0C0Du, // all four bytes non-zero
		0x11223344u, // all four bytes non-zero
		0xDEADBEEFu, // all four bytes non-zero
		0xCAFEBABEu, // all four bytes non-zero
	};
	for (unsigned int x : values)
	{
		REQUIRE(tools_t::convert_string_byte_array_to_uint(tools_t::convert_uint_to_string_byte_array(x)) == x);
	}
}

TEST_CASE("tools_t::trim_cr, no CR present", "[u]")
{
	std::string text = "DEAD";
	REQUIRE(tools_t::trim_cr(text) == "DEAD");
	text = "";
	REQUIRE(tools_t::trim_cr(text) == "");
	text = "no carriage return here";
	REQUIRE(tools_t::trim_cr(text) == "no carriage return here");
}

TEST_CASE("tools_t::replace_non_readable_chars_with_dot, basic cases", "[u]")
{
	SECTION("printable-only string is preserved")
	{
		std::string printable;
		for (int c = 32; c <= 126; ++c)
		{
			printable += static_cast<char>(c);
		}
		REQUIRE(tools_t::replace_non_readable_chars_with_dot(printable) == printable);
	}

	SECTION("non-printable bytes are replaced with dot")
	{
		std::string input = "\x01\x1F\x7F\x80\xFF";
		std::string expected = ".....";
		REQUIRE(tools_t::replace_non_readable_chars_with_dot(input) == expected);
	}
}

TEST_CASE("tools_t::replace_non_readable_chars_with_dot, printable chars preserved", "[u]")
{
	for (int c = 32; c <= 126; ++c)
	{
		std::string single(1, static_cast<char>(c));
		REQUIRE(tools_t::replace_non_readable_chars_with_dot(single) == single);
	}

	std::string all_printable;
	for (int c = 32; c <= 126; ++c)
	{
		all_printable += static_cast<char>(c);
	}
	REQUIRE(tools_t::replace_non_readable_chars_with_dot(all_printable) == all_printable);
}

TEST_CASE("tools_t::replace_non_readable_chars_with_dot, all bytes", "[u]")
{
	for (int i = 0; i <= 255; ++i)
	{
		std::string single(1, static_cast<char>(i));
		std::string result = tools_t::replace_non_readable_chars_with_dot(single);
		if (std::isprint(static_cast<unsigned char>(i)))
		{
			REQUIRE(result == single);
		}
		else
		{
			REQUIRE(result == ".");
		}
	}
}

TEST_CASE("tools_t::initialize_dict, has all expected keys", "[u]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();

	REQUIRE(dict.count(tools_t::rec_type_t::cell) == 1);
	REQUIRE(dict.count(tools_t::rec_type_t::dial) == 1);
	REQUIRE(dict.count(tools_t::rec_type_t::indx) == 1);
	REQUIRE(dict.count(tools_t::rec_type_t::rnam) == 1);
	REQUIRE(dict.count(tools_t::rec_type_t::desc) == 1);
	REQUIRE(dict.count(tools_t::rec_type_t::gmst) == 1);
	REQUIRE(dict.count(tools_t::rec_type_t::fnam) == 1);
	REQUIRE(dict.count(tools_t::rec_type_t::info) == 1);
	REQUIRE(dict.count(tools_t::rec_type_t::text) == 1);
	REQUIRE(dict.count(tools_t::rec_type_t::bnam) == 1);
	REQUIRE(dict.count(tools_t::rec_type_t::sctx) == 1);

	REQUIRE(dict.size() == 11);
}

TEST_CASE("tools_t::initialize_dict, all chapters empty", "[u]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();

	for (const auto & chapter : dict)
	{
		REQUIRE(chapter.second.empty());
	}
}

TEST_CASE("tools_t::get_number_of_elements_in_dict, zero for empty dict", "[u]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	REQUIRE(tools_t::get_number_of_elements_in_dict(dict) == 0);
}

TEST_CASE("tools_t::get_number_of_elements_in_dict, counts inserted entries", "[u]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::cell).insert({ "Balmora", "Balmora", "Balmora", status_t::translated });
	dict.at(tools_t::rec_type_t::cell).insert({ "Vivec", "Vivec", "Vivec", status_t::translated });
	dict.at(tools_t::rec_type_t::dial).insert({ "clanfear", "clanfear", "postrach klanów", status_t::translated });
	REQUIRE(tools_t::get_number_of_elements_in_dict(dict) == 3);
}

TEST_CASE("tools_t::get_number_of_elements_in_dict, correct total", "[u]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::cell).insert({ "Balmora", "Balmora", "Balmora", status_t::translated });
	dict.at(tools_t::rec_type_t::cell).insert({ "Vivec", "Vivec", "Vivec", status_t::translated });
	dict.at(tools_t::rec_type_t::dial).insert({ "clanfear", "clanfear", "postrach klanów", status_t::translated });
	dict.at(tools_t::rec_type_t::info).insert({ "info_key", "info_orig", "info_val", status_t::untranslated });
	REQUIRE(tools_t::get_number_of_elements_in_dict(dict) == 4);
}

TEST_CASE("status_t, all values have distinct non-empty string representations", "[u]")
{
	std::vector<status_t> all_statuses {
		status_t::translated,   status_t::missing,    status_t::duplicate,   status_t::mismatch,
		status_t::error,        status_t::adapted,    status_t::changed,     status_t::reused,
		status_t::untranslated, status_t::ambiguous,  status_t::in_progress, status_t::outdated,
		status_t::model,        status_t::propagated, status_t::heuristic,   status_t::to_verify,
	};

	REQUIRE(all_statuses.size() == 16);

	std::set<std::string_view> unique_set;
	for (const auto & s : all_statuses)
	{
		auto name = status_to_string(s);
		REQUIRE_FALSE(name.empty());
		unique_set.insert(name);
	}

	REQUIRE(unique_set.size() == all_statuses.size());
}

TEST_CASE("tools_t::record_entry_t, speaker fields default to empty", "[u]")
{
	tools_t::record_entry_t entry { "key", "old", "new", status_t::untranslated };
	REQUIRE(entry.speaker_name.empty());
	REQUIRE(entry.gender.empty());
}

TEST_CASE("tools_t::record_entry_t, with speaker fields", "[u]")
{
	tools_t::record_entry_t entry { "key", "old", "new", status_t::translated, "Fargoth", "M" };
	REQUIRE(entry.speaker_name == "Fargoth");
	REQUIRE(entry.gender == "M");
}

TEST_CASE("tools_t::chapter_t::insert, with speaker fields", "[u]")
{
	tools_t::chapter_t chapter;
	bool result = chapter.insert({ "info_1", "Hello", "Cześć", status_t::translated, "Fargoth", "M" });
	REQUIRE(result == true);

	auto * entry = chapter.find("info_1");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->speaker_name == "Fargoth");
	REQUIRE(entry->gender == "M");
}

TEST_CASE("tools_t::initialize_dict, excludes non-dict types", "[u]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	for (const auto & chapter : dict)
	{
		REQUIRE(chapter.first != tools_t::rec_type_t::pgrd);
		REQUIRE(chapter.first != tools_t::rec_type_t::anam);
		REQUIRE(chapter.first != tools_t::rec_type_t::unknown);
	}
}

TEST_CASE("tools_t::chapter_t::find_by_old_text, existing entry", "[u]")
{
	tools_t::chapter_t chapter;
	chapter.insert({ "key1", "Balmora", "Balmora_PL", status_t::translated });
	chapter.insert({ "key2", "Vivec", "Vivec_PL", status_t::translated });

	auto * entry = chapter.find_by_old_text("Balmora");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->key_text == "key1");
	REQUIRE(entry->new_text == "Balmora_PL");
}

TEST_CASE("tools_t::chapter_t::find_by_old_text, missing entry", "[u]")
{
	tools_t::chapter_t chapter;
	chapter.insert({ "key1", "Balmora", "Balmora_PL", status_t::translated });

	auto * entry = chapter.find_by_old_text("Ald-ruhn");
	REQUIRE(entry == nullptr);
}

TEST_CASE("tools_t::chapter_t::find_by_old_text, first-wins semantics", "[u]")
{
	tools_t::chapter_t chapter;
	chapter.insert({ "key1", "Same Text", "Translation A", status_t::translated });
	chapter.insert({ "key2", "Same Text", "Translation B", status_t::translated });

	auto * entry = chapter.find_by_old_text("Same Text");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->key_text == "key1");
	REQUIRE(entry->new_text == "Translation A");
}

TEST_CASE("tools_t::chapter_t::find_by_old_text, empty old_text not indexed", "[u]")
{
	tools_t::chapter_t chapter;
	chapter.insert({ "key1", "", "something", status_t::translated });

	auto * entry = chapter.find_by_old_text("");
	REQUIRE(entry == nullptr);
}

TEST_CASE("file_path_parts_t::set_name, forward slash path", "[u]")
{
	file_path_parts_t n;
	n.set_name("C:/path/to/Morrowind.esm");
	REQUIRE(n.full == "Morrowind.esm");
	REQUIRE(n.name == "Morrowind");
	REQUIRE(n.ext == ".esm");
}

TEST_CASE("file_path_parts_t::set_name, backslash path", "[u]")
{
	file_path_parts_t n;
	n.set_name("C:\\path\\to\\Tribunal.esp");
	REQUIRE(n.full == "Tribunal.esp");
	REQUIRE(n.name == "Tribunal");
	REQUIRE(n.ext == ".esp");
}

TEST_CASE("file_path_parts_t::set_name, multiple dots in filename", "[u]")
{
	file_path_parts_t n;
	n.set_name("C:/path/my.plugin.esp");
	REQUIRE(n.full == "my.plugin.esp");
	REQUIRE(n.name == "my.plugin");
	REQUIRE(n.ext == ".esp");
}

TEST_CASE("file_path_parts_t::set_name, filename only no path", "[u]")
{
	file_path_parts_t n;
	n.set_name("Bloodmoon.json");
	REQUIRE(n.full == "Bloodmoon.json");
	REQUIRE(n.name == "Bloodmoon");
	REQUIRE(n.ext == ".json");
}
