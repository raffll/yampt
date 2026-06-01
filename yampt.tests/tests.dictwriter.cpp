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

    Tools::Dict dict = Tools::initializeDict();
    DictWriter::write(dict, path);

    string content = readTempFile(path);
    REQUIRE(content == "{}");

    removeTempFile(path);
}

TEST_CASE("DictWriter single record serialization", "[dictwriter]")
{
    const string path = "temp_test_writer_single.json";

    Tools::Dict dict = Tools::initializeDict();
    dict[Tools::RecType::CELL].insert({"Seyda Neen", "Seyda Neen", "Sejda Neen", "translated"});

    DictWriter::write(dict, path);

    string content = readTempFile(path);
    nlohmann::json root = nlohmann::json::parse(content);

    REQUIRE(root.contains("CELL"));
    REQUIRE(root["CELL"].is_array());
    REQUIRE(root["CELL"].size() == 1);

    auto & obj = root["CELL"][0];
    REQUIRE(obj["id"] == "Seyda Neen");
    REQUIRE(obj["original"] == "Seyda Neen");
    REQUIRE(obj["translation"] == "Sejda Neen");
    REQUIRE(obj["status"] == "translated");

    removeTempFile(path);
}

TEST_CASE("DictWriter skips empty chapters", "[dictwriter]")
{
    const string path = "temp_test_writer_skip.json";

    Tools::Dict dict = Tools::initializeDict();
    dict[Tools::RecType::INFO].insert({"topic^Hello", "Hello", "Cześć", "translated"});

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

    Tools::Dict dict = Tools::initializeDict();
    dict[Tools::RecType::CELL].insert({"Balmora", "Balmora", "Balmora", "auto_identical"});
    dict[Tools::RecType::CELL].insert({"Vivec", "Vivec", "Vivec PL", "translated"});
    dict[Tools::RecType::INFO].insert({"greeting^Hello traveler", "Hello traveler", "Witaj wędrowcze", "translated"});
    dict[Tools::RecType::INFO].insert({"farewell^Goodbye", "Goodbye", "", "untranslated"});
    dict[Tools::RecType::FNAM].insert({"iron_sword^Iron Sword", "Iron Sword", "Żelazny Miecz", "translated"});
    dict[Tools::RecType::GMST].insert({"sHealth", "Health", "Zdrowie", "translated"});

    DictWriter::write(dict, path);

    DictReader reader(path);
    REQUIRE(reader.isLoaded());

    const auto & loaded = reader.getDict();

    auto * cell1 = loaded.at(Tools::RecType::CELL).find("Balmora");
    REQUIRE(cell1 != nullptr);
    REQUIRE(cell1->id == "Balmora");
    REQUIRE(cell1->original == "Balmora");
    REQUIRE(cell1->translation == "Balmora");
    REQUIRE(cell1->status == "auto_identical");

    auto * cell2 = loaded.at(Tools::RecType::CELL).find("Vivec");
    REQUIRE(cell2 != nullptr);
    REQUIRE(cell2->translation == "Vivec PL");
    REQUIRE(cell2->status == "translated");

    auto * info1 = loaded.at(Tools::RecType::INFO).find("greeting^Hello traveler");
    REQUIRE(info1 != nullptr);
    REQUIRE(info1->original == "Hello traveler");
    REQUIRE(info1->translation == "Witaj wędrowcze");

    auto * info2 = loaded.at(Tools::RecType::INFO).find("farewell^Goodbye");
    REQUIRE(info2 != nullptr);
    REQUIRE(info2->translation == "");
    REQUIRE(info2->status == "untranslated");

    auto * fnam = loaded.at(Tools::RecType::FNAM).find("iron_sword^Iron Sword");
    REQUIRE(fnam != nullptr);
    REQUIRE(fnam->translation == "Żelazny Miecz");

    auto * gmst = loaded.at(Tools::RecType::GMST).find("sHealth");
    REQUIRE(gmst != nullptr);
    REQUIRE(gmst->translation == "Zdrowie");

    removeTempFile(path);
}

TEST_CASE("DictWriter special characters preserved", "[dictwriter]")
{
    const string path = "temp_test_writer_special.json";

    Tools::Dict dict = Tools::initializeDict();
    dict[Tools::RecType::INFO].insert({
        "topic^He said \"hello\"",
        "He said \"hello\"",
        "She said \"goodbye\"",
        "translated"
    });
    dict[Tools::RecType::INFO].insert({
        "topic^Path: C:\\Games\\Morrowind",
        "Path: C:\\Games\\Morrowind",
        "Path: C:\\Gry\\Morrowind",
        "translated"
    });
    dict[Tools::RecType::INFO].insert({
        "topic^Line1\nLine2",
        "Line1\nLine2",
        "Linia1\nLinia2",
        "translated"
    });

    DictWriter::write(dict, path);

    DictReader reader(path);
    REQUIRE(reader.isLoaded());

    const auto & loaded = reader.getDict();

    auto * quotes = loaded.at(Tools::RecType::INFO).find("topic^He said \"hello\"");
    REQUIRE(quotes != nullptr);
    REQUIRE(quotes->original == "He said \"hello\"");
    REQUIRE(quotes->translation == "She said \"goodbye\"");

    auto * backslash = loaded.at(Tools::RecType::INFO).find("topic^Path: C:\\Games\\Morrowind");
    REQUIRE(backslash != nullptr);
    REQUIRE(backslash->translation == "Path: C:\\Gry\\Morrowind");

    auto * newline = loaded.at(Tools::RecType::INFO).find("topic^Line1\nLine2");
    REQUIRE(newline != nullptr);
    REQUIRE(newline->translation == "Linia1\nLinia2");

    removeTempFile(path);
}
