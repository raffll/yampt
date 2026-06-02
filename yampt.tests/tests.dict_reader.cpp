#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dict_reader.hpp"
#include "../yampt/dict_writer.hpp"

#include <fstream>
#include <cstdio>
#include <string>

using namespace std;

static void write_temp_dict(const string & path, const string & content)
{
	ofstream f(path, ios::binary);
	f << content;
}

static void remove_temp_file(const string & path)
{
	remove(path.c_str());
}

TEST_CASE("dict_reader_t valid JSON parsing", "[dictreader]")
{
	const string path = "temp_test_valid.json";
	const string content = R"({
  "CELL": [
    {
      "key": "Balmora",
      "old": "Balmora",
      "new": "Balmora_PL",
      "status": "translated"
    }
  ],
  "INFO": [
    {
      "key": "topic^Hello there, traveler.",
      "old": "Hello there, traveler.",
      "new": "Witaj, wedrowcze.",
      "status": "translated"
    }
  ]
})";

	write_temp_dict(path, content);

	dict_reader_t reader(path);

	REQUIRE(reader.is_loaded() == true);

	auto * cell_entry = reader.get_dict().at(tools_t::rec_type_t::cell).find("Balmora");
	REQUIRE(cell_entry != nullptr);
	REQUIRE(cell_entry->new_text == "Balmora_PL");
	REQUIRE(cell_entry->old_text == "Balmora");
	REQUIRE(cell_entry->status == "translated");

	auto * info_entry = reader.get_dict().at(tools_t::rec_type_t::info).find("topic^Hello there, traveler.");
	REQUIRE(info_entry != nullptr);
	REQUIRE(info_entry->new_text == "Witaj, wedrowcze.");

	remove_temp_file(path);
}

TEST_CASE("dict_reader_t duplicate key keeps first", "[dictreader]")
{
	const string path = "temp_test_dup.json";
	const string content = R"({
  "CELL": [
    {
      "key": "Vivec",
      "old": "Vivec",
      "new": "First",
      "status": "translated"
    },
    {
      "key": "Vivec",
      "old": "Vivec",
      "new": "Second",
      "status": "translated"
    }
  ]
})";

	write_temp_dict(path, content);

	tools_t::reset_log();
	dict_reader_t reader(path);

	REQUIRE(reader.is_loaded() == true);
	REQUIRE(reader.get_dict().at(tools_t::rec_type_t::cell).size() == 1);

	auto * entry = reader.get_dict().at(tools_t::rec_type_t::cell).find("Vivec");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->new_text == "First");

	REQUIRE(tools_t::get_log().find("doubled") != string::npos);

	remove_temp_file(path);
}

TEST_CASE("dict_reader_t unknown rec_type_t is skipped", "[dictreader]")
{
	const string path = "temp_test_bogus.json";
	const string content = R"({
  "BOGUS": [
    {
      "key": "SomeKey",
      "old": "SomeOriginal",
      "new": "SomeTranslation",
      "status": "translated"
    }
  ],
  "CELL": [
    {
      "key": "Ald-ruhn",
      "old": "Ald-ruhn",
      "new": "Ald-ruhn_PL",
      "status": "translated"
    }
  ]
})";

	write_temp_dict(path, content);

	tools_t::reset_log();
	dict_reader_t reader(path);

	REQUIRE(reader.is_loaded() == true);

	auto * cell_entry = reader.get_dict().at(tools_t::rec_type_t::cell).find("Ald-ruhn");
	REQUIRE(cell_entry != nullptr);
	REQUIRE(cell_entry->new_text == "Ald-ruhn_PL");

	REQUIRE(tools_t::get_log().find("unknown rec_type_t") != string::npos);

	remove_temp_file(path);
}

TEST_CASE("dict_reader_t byte limits", "[dictreader]")
{
	const string path = "temp_test_limits.json";

	SECTION("CELL > 63 bytes rejected")
	{
		string long_val(64, 'A');
		string content =
		    R"({"CELL": [{"key": "TestCell", "old": "x", "new": ")" + long_val + R"(", "status": "translated"}]})";
		write_temp_dict(path, content);

		dict_reader_t reader(path);
		REQUIRE(reader.get_dict().at(tools_t::rec_type_t::cell).find("TestCell") == nullptr);

		remove_temp_file(path);
	}

	SECTION("RNAM > 32 bytes rejected")
	{
		string long_val(33, 'B');
		string content =
		    R"({"RNAM": [{"key": "TestRnam", "old": "x", "new": ")" + long_val + R"(", "status": "translated"}]})";
		write_temp_dict(path, content);

		dict_reader_t reader(path);
		REQUIRE(reader.get_dict().at(tools_t::rec_type_t::rnam).find("TestRnam") == nullptr);

		remove_temp_file(path);
	}

	SECTION("FNAM > 31 bytes rejected")
	{
		string long_val(32, 'C');
		string content =
		    R"({"FNAM": [{"key": "TestFnam", "old": "x", "new": ")" + long_val + R"(", "status": "translated"}]})";
		write_temp_dict(path, content);

		dict_reader_t reader(path);
		REQUIRE(reader.get_dict().at(tools_t::rec_type_t::fnam).find("TestFnam") == nullptr);

		remove_temp_file(path);
	}

	SECTION("INFO > 1024 bytes rejected")
	{
		string long_val(1025, 'D');
		string content =
		    R"({"INFO": [{"key": "TestInfo", "old": "x", "new": ")" + long_val + R"(", "status": "translated"}]})";
		write_temp_dict(path, content);

		dict_reader_t reader(path);
		REQUIRE(reader.get_dict().at(tools_t::rec_type_t::info).find("TestInfo") == nullptr);

		remove_temp_file(path);
	}

	SECTION("INFO at exactly 1024 bytes accepted")
	{
		string exact_val(1024, 'E');
		string content =
		    R"({"INFO": [{"key": "TestInfoOk", "old": "x", "new": ")" + exact_val + R"(", "status": "translated"}]})";
		write_temp_dict(path, content);

		dict_reader_t reader(path);
		auto * entry = reader.get_dict().at(tools_t::rec_type_t::info).find("TestInfoOk");
		REQUIRE(entry != nullptr);
		REQUIRE(entry->new_text.size() == 1024);

		remove_temp_file(path);
	}
}

TEST_CASE("dict_reader_t malformed JSON", "[dictreader]")
{
	const string path = "temp_test_malformed.json";
	const string content = "{ \"CELL\": [ { \"id\": \"broken";

	write_temp_dict(path, content);

	dict_reader_t reader(path);

	REQUIRE(reader.is_loaded() == false);

	remove_temp_file(path);
}

TEST_CASE("dict_reader_t missing key field", "[dictreader]")
{
	const string path = "temp_test_noid.json";
	const string content = R"({
  "CELL": [
    {
      "old": "NoIdCell",
      "new": "NoIdTranslation",
      "status": "translated"
    },
    {
      "key": "ValidCell",
      "old": "ValidCell",
      "new": "ValidTranslation",
      "status": "translated"
    }
  ]
})";

	write_temp_dict(path, content);

	tools_t::reset_log();
	dict_reader_t reader(path);

	REQUIRE(reader.is_loaded() == true);
	REQUIRE(reader.get_dict().at(tools_t::rec_type_t::cell).find("NoIdCell") == nullptr);

	auto * valid = reader.get_dict().at(tools_t::rec_type_t::cell).find("ValidCell");
	REQUIRE(valid != nullptr);
	REQUIRE(valid->new_text == "ValidTranslation");

	REQUIRE(tools_t::get_log().find("missing") != string::npos);

	remove_temp_file(path);
}

TEST_CASE("dict_reader_t round-trip with dict_writer_t", "[dictreader]")
{
	const string path = "temp_test_roundtrip.json";

	tools_t::dict_t dict = tools_t::initialize_dict();

	tools_t::record_entry_t cell_entry;
	cell_entry.key_text = "Seyda Neen";
	cell_entry.old_text = "Seyda Neen";
	cell_entry.new_text = "Sejda Neen";
	cell_entry.status = "translated";
	dict.at(tools_t::rec_type_t::cell).insert(cell_entry);

	tools_t::record_entry_t info_entry;
	info_entry.key_text = "topic^Greetings, traveler.";
	info_entry.old_text = "Greetings, traveler.";
	info_entry.new_text = "Witaj, wedrowcze.";
	info_entry.status = "translated";
	dict.at(tools_t::rec_type_t::info).insert(info_entry);

	tools_t::record_entry_t gmst_entry;
	gmst_entry.key_text = "sLevelUp";
	gmst_entry.old_text = "Level Up";
	gmst_entry.new_text = "";
	gmst_entry.status = "untranslated";
	dict.at(tools_t::rec_type_t::gmst).insert(gmst_entry);

	dict_writer_t::write(dict, path);

	dict_reader_t reader(path);

	REQUIRE(reader.is_loaded() == true);

	auto * cell = reader.get_dict().at(tools_t::rec_type_t::cell).find("Seyda Neen");
	REQUIRE(cell != nullptr);
	REQUIRE(cell->old_text == "Seyda Neen");
	REQUIRE(cell->new_text == "Sejda Neen");
	REQUIRE(cell->status == "translated");

	auto * info = reader.get_dict().at(tools_t::rec_type_t::info).find("topic^Greetings, traveler.");
	REQUIRE(info != nullptr);
	REQUIRE(info->old_text == "Greetings, traveler.");
	REQUIRE(info->new_text == "Witaj, wedrowcze.");
	REQUIRE(info->status == "translated");

	auto * gmst = reader.get_dict().at(tools_t::rec_type_t::gmst).find("sLevelUp");
	REQUIRE(gmst != nullptr);
	REQUIRE(gmst->old_text == "Level Up");
	REQUIRE(gmst->new_text == "");
	REQUIRE(gmst->status == "untranslated");

	remove_temp_file(path);
}
