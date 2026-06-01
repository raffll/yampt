#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dictwriter.hpp"
#include "../yampt/dictreader.hpp"

#include <fstream>
#include <cstdio>

using namespace std;

static string readTempFile(const string & path)
{
    ifstream f(path, ios::binary);
    return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
}

static void removeTempFile(const string & path)
{
    remove(path.c_str());
}

TEST_CASE("DictWriter empty dict produces valid JSON object", "[dictwriter]")
{
    const string path = "temp_test_writer_empty.json";

    tools_t::dict_t dict = tools_t::initializeDict();
    DictWriter::write(dict, path);

    string content = readTempFile(path);
    REQUIRE(content == "{}");

    removeTempFile(path);
}

TEST_CASE("DictWriter single record serialization", "[dictwriter]")
{
    const string path = "temp_test_writer_single.json";

    tools_t::dict_t dict = tools_t::initializeDict();
    dict[tools_t::rec_type_t::CELL].insert({"Seyda Neen", "Seyda Neen", "Sejda Neen", "translated"});

    DictWriter::write(dict, path);

    string content = readTempFile(path);
    nlohmann::json root = nlohmann::json::parse(content);

    REQUIRE(root.contains("CELL"));
    REQUIRE(root["CELL"].is_array());
    REQUIRE(root["CELL"].size() == 1);

    auto & obj = root["CELL"][0];
    REQUIRE(obj["id"] == "Seyda Neen");
    REQUIRE(obj["old"] == "Seyda Neen");
    REQUIRE(obj["new"] == "Sejda Neen");
    REQUIRE(obj["status"] == "translated");

    removeTempFile(path);
}

TEST_CASE("DictWriter skips empty chapters", "[dictwriter]")
{
    const string path = "temp_test_writer_skip.json";

    tools_t::dict_t dict = tools_t::initializeDict();
    dict[tools_t::rec_type_t::INFO].insert({"topic^Hello", "Hello", "Cześć", "translated"});

    DictWriter::write(dict, path);

    string content = readTempFile(path);
    nlohmann::json root = nlohmann::json::parse(content);

    REQUIRE(root.contains("INFO"));
    REQUIRE_FALSE(root.contains("CELL"));
    REQUIRE_FALSE(root.contains("DIAL"));
    REQUIRE_FALSE(root.contains("INDX"));
    REQUIRE_FALSE(root.contains("RNAM"));
    REQUIRE_FALSE(root.contains("DESC"));
    REQUIRE_FALSE(root.contains("GMST"));
    REQUIRE_FALSE(root.contains("FNAM"));

    removeTempFile(path);
}

TEST_CASE("DictWriter round-trip with DictReader", "[dictwriter]")
{
    const string path = "temp_test_writer_roundtrip.json";

    tools_t::dict_t dict = tools_t::initializeDict();
    dict[tools_t::rec_type_t::CELL].insert({"Balmora", "Balmora", "Balmora", "auto_identical"});
    dict[tools_t::rec_type_t::CELL].insert({"Vivec", "Vivec", "Vivec PL", "translated"});
    dict[tools_t::rec_type_t::INFO].insert({"greeting^Hello traveler", "Hello traveler", "Witaj wędrowcze", "translated"});
    dict[tools_t::rec_type_t::INFO].insert({"farewell^Goodbye", "Goodbye", "", "untranslated"});
    dict[tools_t::rec_type_t::FNAM].insert({"iron_sword^Iron Sword", "Iron Sword", "Żelazny Miecz", "translated"});
    dict[tools_t::rec_type_t::GMST].insert({"sHealth", "Health", "Zdrowie", "translated"});

    DictWriter::write(dict, path);

    DictReader reader(path);
    REQUIRE(reader.isLoaded());

    const auto & loaded = reader.getDict();

    auto * cell1 = loaded.at(tools_t::rec_type_t::CELL).find("Balmora");
    REQUIRE(cell1 != nullptr);
    REQUIRE(cell1->key_text == "Balmora");
    REQUIRE(cell1->old_text == "Balmora");
    REQUIRE(cell1->new_text == "Balmora");
    REQUIRE(cell1->status == "auto_identical");

    auto * cell2 = loaded.at(tools_t::rec_type_t::CELL).find("Vivec");
    REQUIRE(cell2 != nullptr);
    REQUIRE(cell2->new_text == "Vivec PL");
    REQUIRE(cell2->status == "translated");

    auto * info1 = loaded.at(tools_t::rec_type_t::INFO).find("greeting^Hello traveler");
    REQUIRE(info1 != nullptr);
    REQUIRE(info1->old_text == "Hello traveler");
    REQUIRE(info1->new_text == "Witaj wędrowcze");

    auto * info2 = loaded.at(tools_t::rec_type_t::INFO).find("farewell^Goodbye");
    REQUIRE(info2 != nullptr);
    REQUIRE(info2->new_text == "");
    REQUIRE(info2->status == "untranslated");

    auto * fnam = loaded.at(tools_t::rec_type_t::FNAM).find("iron_sword^Iron Sword");
    REQUIRE(fnam != nullptr);
    REQUIRE(fnam->new_text == "Żelazny Miecz");

    auto * gmst = loaded.at(tools_t::rec_type_t::GMST).find("sHealth");
    REQUIRE(gmst != nullptr);
    REQUIRE(gmst->new_text == "Zdrowie");

    removeTempFile(path);
}

TEST_CASE("DictWriter special characters preserved", "[dictwriter]")
{
    const string path = "temp_test_writer_special.json";

    tools_t::dict_t dict = tools_t::initializeDict();
    dict[tools_t::rec_type_t::INFO].insert({
        "topic^He said \"hello\"",
        "He said \"hello\"",
        "She said \"goodbye\"",
        "translated"
    });
    dict[tools_t::rec_type_t::INFO].insert({
        "topic^Path: C:\\Games\\Morrowind",
        "Path: C:\\Games\\Morrowind",
        "Path: C:\\Gry\\Morrowind",
        "translated"
    });
    dict[tools_t::rec_type_t::INFO].insert({
        "topic^Line1\nLine2",
        "Line1\nLine2",
        "Linia1\nLinia2",
        "translated"
    });

    DictWriter::write(dict, path);

    DictReader reader(path);
    REQUIRE(reader.isLoaded());

    const auto & loaded = reader.getDict();

    auto * quotes = loaded.at(tools_t::rec_type_t::INFO).find("topic^He said \"hello\"");
    REQUIRE(quotes != nullptr);
    REQUIRE(quotes->old_text == "He said \"hello\"");
    REQUIRE(quotes->new_text == "She said \"goodbye\"");

    auto * backslash = loaded.at(tools_t::rec_type_t::INFO).find("topic^Path: C:\\Games\\Morrowind");
    REQUIRE(backslash != nullptr);
    REQUIRE(backslash->new_text == "Path: C:\\Gry\\Morrowind");

    auto * newline = loaded.at(tools_t::rec_type_t::INFO).find("topic^Line1\nLine2");
    REQUIRE(newline != nullptr);
    REQUIRE(newline->new_text == "Linia1\nLinia2");

    removeTempFile(path);
}
