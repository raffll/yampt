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
      "original": "Balmora",
      "translation": "Balmora_PL",
      "status": "translated"
    }
  ],
  "INFO": [
    {
      "id": "topic^Hello there, traveler.",
      "original": "Hello there, traveler.",
      "translation": "Witaj, wedrowcze.",
      "status": "translated"
    }
  ]
})";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);

    auto * cell_entry = reader.getDict().at(Tools::RecType::CELL).find("Balmora");
    REQUIRE(cell_entry != nullptr);
    REQUIRE(cell_entry->translation == "Balmora_PL");
    REQUIRE(cell_entry->original == "Balmora");
    REQUIRE(cell_entry->status == "translated");

    auto * info_entry = reader.getDict().at(Tools::RecType::INFO).find("topic^Hello there, traveler.");
    REQUIRE(info_entry != nullptr);
    REQUIRE(info_entry->translation == "Witaj, wedrowcze.");

    removeTempFile(path);
}

TEST_CASE("DictReader duplicate id keeps first", "[dictreader]")
{
    const string path = "temp_test_dup.json";
    const string content = R"({
  "CELL": [
    {
      "id": "Vivec",
      "original": "Vivec",
      "translation": "First",
      "status": "translated"
    },
    {
      "id": "Vivec",
      "original": "Vivec",
      "translation": "Second",
      "status": "translated"
    }
  ]
})";

    writeTempDict(path, content);

    Tools::resetLog();
    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);
    REQUIRE(reader.getDict().at(Tools::RecType::CELL).size() == 1);

    auto * entry = reader.getDict().at(Tools::RecType::CELL).find("Vivec");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->translation == "First");

    REQUIRE(Tools::getLog().find("doubled") != string::npos);

    removeTempFile(path);
}

TEST_CASE("DictReader unknown RecType is skipped", "[dictreader]")
{
    const string path = "temp_test_bogus.json";
    const string content = R"({
  "BOGUS": [
    {
      "id": "SomeKey",
      "original": "SomeOriginal",
      "translation": "SomeTranslation",
      "status": "translated"
    }
  ],
  "CELL": [
    {
      "id": "Ald-ruhn",
      "original": "Ald-ruhn",
      "translation": "Ald-ruhn_PL",
      "status": "translated"
    }
  ]
})";

    writeTempDict(path, content);

    Tools::resetLog();
    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);

    auto * cell_entry = reader.getDict().at(Tools::RecType::CELL).find("Ald-ruhn");
    REQUIRE(cell_entry != nullptr);
    REQUIRE(cell_entry->translation == "Ald-ruhn_PL");

    REQUIRE(Tools::getLog().find("unknown RecType") != string::npos);

    removeTempFile(path);
}

TEST_CASE("DictReader byte limits", "[dictreader]")
{
    const string path = "temp_test_limits.json";

    SECTION("CELL > 63 bytes rejected")
    {
        string long_val(64, 'A');
        string content = R"({"CELL": [{"id": "TestCell", "original": "x", "translation": ")" + long_val + R"(", "status": "translated"}]})";
        writeTempDict(path, content);

        DictReader reader(path);
        REQUIRE(reader.getDict().at(Tools::RecType::CELL).find("TestCell") == nullptr);

        removeTempFile(path);
    }

    SECTION("RNAM > 32 bytes rejected")
    {
        string long_val(33, 'B');
        string content = R"({"RNAM": [{"id": "TestRnam", "original": "x", "translation": ")" + long_val + R"(", "status": "translated"}]})";
        writeTempDict(path, content);

        DictReader reader(path);
        REQUIRE(reader.getDict().at(Tools::RecType::RNAM).find("TestRnam") == nullptr);

        removeTempFile(path);
    }

    SECTION("FNAM > 31 bytes rejected")
    {
        string long_val(32, 'C');
        string content = R"({"FNAM": [{"id": "TestFnam", "original": "x", "translation": ")" + long_val + R"(", "status": "translated"}]})";
        writeTempDict(path, content);

        DictReader reader(path);
        REQUIRE(reader.getDict().at(Tools::RecType::FNAM).find("TestFnam") == nullptr);

        removeTempFile(path);
    }

    SECTION("INFO > 1024 bytes rejected")
    {
        string long_val(1025, 'D');
        string content = R"({"INFO": [{"id": "TestInfo", "original": "x", "translation": ")" + long_val + R"(", "status": "translated"}]})";
        writeTempDict(path, content);

        DictReader reader(path);
        REQUIRE(reader.getDict().at(Tools::RecType::INFO).find("TestInfo") == nullptr);

        removeTempFile(path);
    }

    SECTION("INFO at exactly 1024 bytes accepted")
    {
        string exact_val(1024, 'E');
        string content = R"({"INFO": [{"id": "TestInfoOk", "original": "x", "translation": ")" + exact_val + R"(", "status": "translated"}]})";
        writeTempDict(path, content);

        DictReader reader(path);
        auto * entry = reader.getDict().at(Tools::RecType::INFO).find("TestInfoOk");
        REQUIRE(entry != nullptr);
        REQUIRE(entry->translation.size() == 1024);

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
      "original": "NoIdCell",
      "translation": "NoIdTranslation",
      "status": "translated"
    },
    {
      "id": "ValidCell",
      "original": "ValidCell",
      "translation": "ValidTranslation",
      "status": "translated"
    }
  ]
})";

    writeTempDict(path, content);

    Tools::resetLog();
    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);
    REQUIRE(reader.getDict().at(Tools::RecType::CELL).find("NoIdCell") == nullptr);

    auto * valid = reader.getDict().at(Tools::RecType::CELL).find("ValidCell");
    REQUIRE(valid != nullptr);
    REQUIRE(valid->translation == "ValidTranslation");

    REQUIRE(Tools::getLog().find("missing") != string::npos);

    removeTempFile(path);
}

TEST_CASE("DictReader round-trip with DictWriter", "[dictreader]")
{
    const string path = "temp_test_roundtrip.json";

    Tools::Dict dict = Tools::initializeDict();

    Tools::RecordEntry cell_entry;
    cell_entry.id = "Seyda Neen";
    cell_entry.original = "Seyda Neen";
    cell_entry.translation = "Sejda Neen";
    cell_entry.status = "translated";
    dict.at(Tools::RecType::CELL).insert(cell_entry);

    Tools::RecordEntry info_entry;
    info_entry.id = "topic^Greetings, traveler.";
    info_entry.original = "Greetings, traveler.";
    info_entry.translation = "Witaj, wedrowcze.";
    info_entry.status = "translated";
    dict.at(Tools::RecType::INFO).insert(info_entry);

    Tools::RecordEntry gmst_entry;
    gmst_entry.id = "sLevelUp";
    gmst_entry.original = "Level Up";
    gmst_entry.translation = "";
    gmst_entry.status = "untranslated";
    dict.at(Tools::RecType::GMST).insert(gmst_entry);

    DictWriter::write(dict, path);

    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);

    auto * cell = reader.getDict().at(Tools::RecType::CELL).find("Seyda Neen");
    REQUIRE(cell != nullptr);
    REQUIRE(cell->original == "Seyda Neen");
    REQUIRE(cell->translation == "Sejda Neen");
    REQUIRE(cell->status == "translated");

    auto * info = reader.getDict().at(Tools::RecType::INFO).find("topic^Greetings, traveler.");
    REQUIRE(info != nullptr);
    REQUIRE(info->original == "Greetings, traveler.");
    REQUIRE(info->translation == "Witaj, wedrowcze.");
    REQUIRE(info->status == "translated");

    auto * gmst = reader.getDict().at(Tools::RecType::GMST).find("sLevelUp");
    REQUIRE(gmst != nullptr);
    REQUIRE(gmst->original == "Level Up");
    REQUIRE(gmst->translation == "");
    REQUIRE(gmst->status == "untranslated");

    removeTempFile(path);
}
