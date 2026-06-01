#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dictmerger.hpp"

#include <fstream>
#include <cstdio>

using namespace std;

static void writeTempDict(const std::string & path, const std::string & content)
{
    std::ofstream f(path, std::ios::binary);
    f << content;
}

static void removeTempFile(const std::string & path)
{
    std::remove(path.c_str());
}

TEST_CASE("DictMerger addRecord inserts entry", "[u]")
{
    DictMerger merger;
    merger.addRecord(Tools::RecType::CELL, "Balmora", "Balmora");
    const auto & dict = merger.getDict();
    const auto & chapter = dict.at(Tools::RecType::CELL);
    REQUIRE(chapter.find("Balmora") != chapter.end());
    REQUIRE(chapter.at("Balmora") == "Balmora");
}

TEST_CASE("DictMerger merge first-wins precedence", "[i]")
{
    const std::string path1 = "temp_merger_first.yml";
    const std::string path2 = "temp_merger_second.yml";

    writeTempDict(path1,
        "<record>\r\n"
        "\t<_id>CELL</_id>\r\n"
        "\t<key>Balmora</key>\r\n"
        "\t<val>FirstValue</val>\r\n"
        "</record>\r\n");

    writeTempDict(path2,
        "<record>\r\n"
        "\t<_id>CELL</_id>\r\n"
        "\t<key>Balmora</key>\r\n"
        "\t<val>SecondValue</val>\r\n"
        "</record>\r\n");

    DictMerger merger({ path1, path2 });
    const auto & chapter = merger.getDict().at(Tools::RecType::CELL);
    REQUIRE(chapter.find("Balmora") != chapter.end());
    REQUIRE(chapter.at("Balmora") == "FirstValue");

    removeTempFile(path1);
    removeTempFile(path2);
}

TEST_CASE("DictMerger key only in second dict", "[i]")
{
    const std::string path1 = "temp_merger_nokey1.yml";
    const std::string path2 = "temp_merger_nokey2.yml";

    writeTempDict(path1,
        "<record>\r\n"
        "\t<_id>CELL</_id>\r\n"
        "\t<key>Vivec</key>\r\n"
        "\t<val>Vivec</val>\r\n"
        "</record>\r\n");

    writeTempDict(path2,
        "<record>\r\n"
        "\t<_id>CELL</_id>\r\n"
        "\t<key>Balmora</key>\r\n"
        "\t<val>Balmora</val>\r\n"
        "</record>\r\n");

    DictMerger merger({ path1, path2 });
    const auto & chapter = merger.getDict().at(Tools::RecType::CELL);
    REQUIRE(chapter.find("Balmora") != chapter.end());
    REQUIRE(chapter.at("Balmora") == "Balmora");

    removeTempFile(path1);
    removeTempFile(path2);
}

TEST_CASE("DictMerger identical values", "[i]")
{
    const std::string path1 = "temp_merger_ident1.yml";
    const std::string path2 = "temp_merger_ident2.yml";

    writeTempDict(path1,
        "<record>\r\n"
        "\t<_id>CELL</_id>\r\n"
        "\t<key>Balmora</key>\r\n"
        "\t<val>Balmora</val>\r\n"
        "</record>\r\n");

    writeTempDict(path2,
        "<record>\r\n"
        "\t<_id>CELL</_id>\r\n"
        "\t<key>Balmora</key>\r\n"
        "\t<val>Balmora</val>\r\n"
        "</record>\r\n");

    DictMerger merger({ path1, path2 });
    const auto & chapter = merger.getDict().at(Tools::RecType::CELL);
    REQUIRE(chapter.find("Balmora") != chapter.end());
    REQUIRE(chapter.count("Balmora") == 1);
    REQUIRE(chapter.at("Balmora") == "Balmora");

    removeTempFile(path1);
    removeTempFile(path2);
}
