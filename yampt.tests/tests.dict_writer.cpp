#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dict_writer.hpp"
#include "../yampt/dict_reader.hpp"

#include <fstream>
#include <cstdio>

using namespace std;

static string read_temp_file(const string & path)
{
	ifstream f(path, ios::binary);
	return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
}

static void remove_temp_file(const string & path)
{
	remove(path.c_str());
}

TEST_CASE("dict_writer_t empty dict produces valid JSON object", "[dictwriter]")
{
	const string path = "temp_test_writer_empty.json";

	tools_t::dict_t dict = tools_t::initialize_dict();
	dict_writer_t::write(dict, path);

	string content = read_temp_file(path);
	REQUIRE(content == "{}");

	remove_temp_file(path);
}

TEST_CASE("dict_writer_t single record serialization", "[dictwriter]")
{
	const string path = "temp_test_writer_single.json";

	tools_t::dict_t dict = tools_t::initialize_dict();
	dict[tools_t::rec_type_t::cell].insert({ "Seyda Neen", "Seyda Neen", "Sejda Neen", "translated" });

	dict_writer_t::write(dict, path);

	string content = read_temp_file(path);
	nlohmann::json root = nlohmann::json::parse(content);

	REQUIRE(root.contains("CELL"));
	REQUIRE(root["CELL"].is_array());
	REQUIRE(root["CELL"].size() == 1);

	auto & obj = root["CELL"][0];
	REQUIRE(obj["key"] == "Seyda Neen");
	REQUIRE(obj["old"] == "Seyda Neen");
	REQUIRE(obj["new"] == "Sejda Neen");
	REQUIRE(obj["status"] == "translated");

	remove_temp_file(path);
}

TEST_CASE("dict_writer_t skips empty chapters", "[dictwriter]")
{
	const string path = "temp_test_writer_skip.json";

	tools_t::dict_t dict = tools_t::initialize_dict();
	dict[tools_t::rec_type_t::info].insert({ "topic^Hello", "Hello", "Cześć", "translated" });

	dict_writer_t::write(dict, path);

	string content = read_temp_file(path);
	nlohmann::json root = nlohmann::json::parse(content);

	REQUIRE(root.contains("INFO"));
	REQUIRE_FALSE(root.contains("CELL"));
	REQUIRE_FALSE(root.contains("DIAL"));
	REQUIRE_FALSE(root.contains("INDX"));
	REQUIRE_FALSE(root.contains("RNAM"));
	REQUIRE_FALSE(root.contains("DESC"));
	REQUIRE_FALSE(root.contains("GMST"));
	REQUIRE_FALSE(root.contains("FNAM"));

	remove_temp_file(path);
}

TEST_CASE("dict_writer_t round-trip with dict_reader_t", "[dictwriter]")
{
	const string path = "temp_test_writer_roundtrip.json";

	tools_t::dict_t dict = tools_t::initialize_dict();
	dict[tools_t::rec_type_t::cell].insert({ "Balmora", "Balmora", "Balmora", "auto_identical" });
	dict[tools_t::rec_type_t::cell].insert({ "Vivec", "Vivec", "Vivec PL", "translated" });
	dict[tools_t::rec_type_t::info].insert(
	    { "greeting^Hello traveler", "Hello traveler", "Witaj wędrowcze", "translated" });
	dict[tools_t::rec_type_t::info].insert({ "farewell^Goodbye", "Goodbye", "", "untranslated" });
	dict[tools_t::rec_type_t::fnam].insert({ "iron_sword^Iron Sword", "Iron Sword", "Żelazny Miecz", "translated" });
	dict[tools_t::rec_type_t::gmst].insert({ "sHealth", "Health", "Zdrowie", "translated" });

	dict_writer_t::write(dict, path);

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());

	const auto & loaded = reader.get_dict();

	auto * cell1 = loaded.at(tools_t::rec_type_t::cell).find("Balmora");
	REQUIRE(cell1 != nullptr);
	REQUIRE(cell1->key_text == "Balmora");
	REQUIRE(cell1->old_text == "Balmora");
	REQUIRE(cell1->new_text == "Balmora");
	REQUIRE(cell1->status == "auto_identical");

	auto * cell2 = loaded.at(tools_t::rec_type_t::cell).find("Vivec");
	REQUIRE(cell2 != nullptr);
	REQUIRE(cell2->new_text == "Vivec PL");
	REQUIRE(cell2->status == "translated");

	auto * info1 = loaded.at(tools_t::rec_type_t::info).find("greeting^Hello traveler");
	REQUIRE(info1 != nullptr);
	REQUIRE(info1->old_text == "Hello traveler");
	REQUIRE(info1->new_text == "Witaj wędrowcze");

	auto * info2 = loaded.at(tools_t::rec_type_t::info).find("farewell^Goodbye");
	REQUIRE(info2 != nullptr);
	REQUIRE(info2->new_text == "");
	REQUIRE(info2->status == "untranslated");

	auto * fnam = loaded.at(tools_t::rec_type_t::fnam).find("iron_sword^Iron Sword");
	REQUIRE(fnam != nullptr);
	REQUIRE(fnam->new_text == "Żelazny Miecz");

	auto * gmst = loaded.at(tools_t::rec_type_t::gmst).find("sHealth");
	REQUIRE(gmst != nullptr);
	REQUIRE(gmst->new_text == "Zdrowie");

	remove_temp_file(path);
}

TEST_CASE("dict_writer_t special characters preserved", "[dictwriter]")
{
	const string path = "temp_test_writer_special.json";

	tools_t::dict_t dict = tools_t::initialize_dict();
	dict[tools_t::rec_type_t::info].insert(
	    { "topic^He said \"hello\"", "He said \"hello\"", "She said \"goodbye\"", "translated" });
	dict[tools_t::rec_type_t::info].insert(
	    { "topic^Path: C:\\Games\\Morrowind", "Path: C:\\Games\\Morrowind", "Path: C:\\Gry\\Morrowind", "translated" });
	dict[tools_t::rec_type_t::info].insert({ "topic^Line1\nLine2", "Line1\nLine2", "Linia1\nLinia2", "translated" });

	dict_writer_t::write(dict, path);

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());

	const auto & loaded = reader.get_dict();

	auto * quotes = loaded.at(tools_t::rec_type_t::info).find("topic^He said \"hello\"");
	REQUIRE(quotes != nullptr);
	REQUIRE(quotes->old_text == "He said \"hello\"");
	REQUIRE(quotes->new_text == "She said \"goodbye\"");

	auto * backslash = loaded.at(tools_t::rec_type_t::info).find("topic^Path: C:\\Games\\Morrowind");
	REQUIRE(backslash != nullptr);
	REQUIRE(backslash->new_text == "Path: C:\\Gry\\Morrowind");

	auto * newline = loaded.at(tools_t::rec_type_t::info).find("topic^Line1\nLine2");
	REQUIRE(newline != nullptr);
	REQUIRE(newline->new_text == "Linia1\nLinia2");

	remove_temp_file(path);
}
