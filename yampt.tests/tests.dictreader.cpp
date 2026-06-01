#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dictreader.hpp"
#include "../yampt/dictwriter.hpp"

#include <fstream>
#include <cstdio>
#include <string>

using namespace std;

static void writeTempDict(const string & path, const string & content)
{
    ofstream f(path, ios::binary);
    f << content;
}

static void removeTempFile(const string & path)
{
    remove(path.c_str());
}

TEST_CASE("DictReader valid JSON parsing", "[dictreader]")
{
    const string path = "temp_test_valid.json";
    const string content = R"({
  "CELL": [
    {
      "id": "Balmora",
      "old": "Balmora",
      "new": "Balmora_PL",
      "status": "translated"
    }
  ],
  "INFO": [
    {
      "id": "topic^Hello there, traveler.",
      "old": "Hello there, traveler.",
      "new": "Witaj, wedrowcze.",
      "status": "translated"
    }
  ]
})";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);

    auto * cell_entry = reader.getDict().at(tools_t::rec_type_t::CELL).find("Balmora");
    REQUIRE(cell_entry != nullptr);
    REQUIRE(cell_entry->new_text == "Balmora_PL");
    REQUIRE(cell_entry->old_text == "Balmora");
    REQUIRE(cell_entry->status == "translated");

    auto * info_entry = reader.getDict().at(tools_t::rec_type_t::INFO).find("topic^Hello there, traveler.");
    REQUIRE(info_entry != nullptr);
    REQUIRE(info_entry->new_text == "Witaj, wedrowcze.");

    removeTempFile(path);
}

TEST_CASE("DictReader duplicate id keeps first", "[dictreader]")
{
    const string path = "temp_test_dup.json";
    const string content = R"({
  "CELL": [
    {
      "id": "Vivec",
      "old": "Vivec",
      "new": "First",
      "status": "translated"
    },
    {
      "id": "Vivec",
      "old": "Vivec",
      "new": "Second",
      "status": "translated"
    }
  ]
})";

    writeTempDict(path, content);

    tools_t::resetLog();
    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);
    REQUIRE(reader.getDict().at(tools_t::rec_type_t::CELL).size() == 1);

    auto * entry = reader.getDict().at(tools_t::rec_type_t::CELL).find("Vivec");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->new_text == "First");

    REQUIRE(tools_t::getLog().find("doubled") != string::npos);

    removeTempFile(path);
}

TEST_CASE("DictReader unknown rec_type_t is skipped", "[dictreader]")
{
    const string path = "temp_test_bogus.json";
    const string content = R"({
  "BOGUS": [
    {
      "id": "SomeKey",
      "old": "SomeOriginal",
      "new": "SomeTranslation",
      "status": "translated"
    }
  ],
  "CELL": [
    {
      "id": "Ald-ruhn",
      "old": "Ald-ruhn",
      "new": "Ald-ruhn_PL",
      "status": "translated"
    }
  ]
})";

    writeTempDict(path, content);

    tools_t::resetLog();
    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);

    auto * cell_entry = reader.getDict().at(tools_t::rec_type_t::CELL).find("Ald-ruhn");
    REQUIRE(cell_entry != nullptr);
    REQUIRE(cell_entry->new_text == "Ald-ruhn_PL");

    REQUIRE(tools_t::getLog().find("unknown rec_type_t") != string::npos);

    removeTempFile(path);
}

TEST_CASE("DictReader byte limits", "[dictreader]")
{
    const string path = "temp_test_limits.json";

    SECTION("CELL > 63 bytes rejected")
    {
        string long_val(64, 'A');
        string content = R"({"CELL": [{"id": "TestCell", "old": "x", "new": ")" + long_val + R"(", "status": "translated"}]})";
        writeTempDict(path, content);

        DictReader reader(path);
        REQUIRE(reader.getDict().at(tools_t::rec_type_t::CELL).find("TestCell") == nullptr);

        removeTempFile(path);
    }

    SECTION("RNAM > 32 bytes rejected")
    {
        string long_val(33, 'B');
        string content = R"({"RNAM": [{"id": "TestRnam", "old": "x", "new": ")" + long_val + R"(", "status": "translated"}]})";
        writeTempDict(path, content);

        DictReader reader(path);
        REQUIRE(reader.getDict().at(tools_t::rec_type_t::RNAM).find("TestRnam") == nullptr);

        removeTempFile(path);
    }

    SECTION("FNAM > 31 bytes rejected")
    {
        string long_val(32, 'C');
        string content = R"({"FNAM": [{"id": "TestFnam", "old": "x", "new": ")" + long_val + R"(", "status": "translated"}]})";
        writeTempDict(path, content);

        DictReader reader(path);
        REQUIRE(reader.getDict().at(tools_t::rec_type_t::FNAM).find("TestFnam") == nullptr);

        removeTempFile(path);
    }

    SECTION("INFO > 1024 bytes rejected")
    {
        string long_val(1025, 'D');
        string content = R"({"INFO": [{"id": "TestInfo", "old": "x", "new": ")" + long_val + R"(", "status": "translated"}]})";
        writeTempDict(path, content);

        DictReader reader(path);
        REQUIRE(reader.getDict().at(tools_t::rec_type_t::INFO).find("TestInfo") == nullptr);

        removeTempFile(path);
    }

    SECTION("INFO at exactly 1024 bytes accepted")
    {
        string exact_val(1024, 'E');
        string content = R"({"INFO": [{"id": "TestInfoOk", "old": "x", "new": ")" + exact_val + R"(", "status": "translated"}]})";
        writeTempDict(path, content);

        DictReader reader(path);
        auto * entry = reader.getDict().at(tools_t::rec_type_t::INFO).find("TestInfoOk");
        REQUIRE(entry != nullptr);
        REQUIRE(entry->new_text.size() == 1024);

        removeTempFile(path);
    }
}

TEST_CASE("DictReader malformed JSON", "[dictreader]")
{
    const string path = "temp_test_malformed.json";
    const string content = "{ \"CELL\": [ { \"id\": \"broken";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.isLoaded() == false);

    removeTempFile(path);
}

TEST_CASE("DictReader missing id field", "[dictreader]")
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
      "id": "ValidCell",
      "old": "ValidCell",
      "new": "ValidTranslation",
      "status": "translated"
    }
  ]
})";

    writeTempDict(path, content);

    tools_t::resetLog();
    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);
    REQUIRE(reader.getDict().at(tools_t::rec_type_t::CELL).find("NoIdCell") == nullptr);

    auto * valid = reader.getDict().at(tools_t::rec_type_t::CELL).find("ValidCell");
    REQUIRE(valid != nullptr);
    REQUIRE(valid->new_text == "ValidTranslation");

    REQUIRE(tools_t::getLog().find("missing") != string::npos);

    removeTempFile(path);
}

TEST_CASE("DictReader round-trip with DictWriter", "[dictreader]")
{
    const string path = "temp_test_roundtrip.json";

    tools_t::dict_t dict = tools_t::initializeDict();

    tools_t::RecordEntry cell_entry;
    cell_entry.key_text = "Seyda Neen";
    cell_entry.old_text = "Seyda Neen";
    cell_entry.new_text = "Sejda Neen";
    cell_entry.status = "translated";
    dict.at(tools_t::rec_type_t::CELL).insert(cell_entry);

    tools_t::RecordEntry info_entry;
    info_entry.key_text = "topic^Greetings, traveler.";
    info_entry.old_text = "Greetings, traveler.";
    info_entry.new_text = "Witaj, wedrowcze.";
    info_entry.status = "translated";
    dict.at(tools_t::rec_type_t::INFO).insert(info_entry);

    tools_t::RecordEntry gmst_entry;
    gmst_entry.key_text = "sLevelUp";
    gmst_entry.old_text = "Level Up";
    gmst_entry.new_text = "";
    gmst_entry.status = "untranslated";
    dict.at(tools_t::rec_type_t::GMST).insert(gmst_entry);

    DictWriter::write(dict, path);

    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);

    auto * cell = reader.getDict().at(tools_t::rec_type_t::CELL).find("Seyda Neen");
    REQUIRE(cell != nullptr);
    REQUIRE(cell->old_text == "Seyda Neen");
    REQUIRE(cell->new_text == "Sejda Neen");
    REQUIRE(cell->status == "translated");

    auto * info = reader.getDict().at(tools_t::rec_type_t::INFO).find("topic^Greetings, traveler.");
    REQUIRE(info != nullptr);
    REQUIRE(info->old_text == "Greetings, traveler.");
    REQUIRE(info->new_text == "Witaj, wedrowcze.");
    REQUIRE(info->status == "translated");

    auto * gmst = reader.getDict().at(tools_t::rec_type_t::GMST).find("sLevelUp");
    REQUIRE(gmst != nullptr);
    REQUIRE(gmst->old_text == "Level Up");
    REQUIRE(gmst->new_text == "");
    REQUIRE(gmst->status == "untranslated");

    removeTempFile(path);
}
