#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dictmerger.hpp"

#include <fstream>
#include <cstdio>

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
    merger.addRecord(tools_t::rec_type_t::CELL, "Balmora", "BalmoraTrans");
    const auto & dict = merger.getDict();
    const auto * entry = dict.at(tools_t::rec_type_t::CELL).find("Balmora");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->new_text == "BalmoraTrans");
}

TEST_CASE("DictMerger merge first-wins precedence", "[i]")
{
    const std::string path1 = "temp_merger_first.json";
    const std::string path2 = "temp_merger_second.json";

    tools_t::resetLog();

    writeTempDict(path1,
        "{\n"
        "  \"CELL\": [\n"
        "    { \"id\": \"Balmora\", \"old\": \"\", \"new\": \"FirstValue\", \"status\": \"translated\" }\n"
        "  ]\n"
        "}\n");

    writeTempDict(path2,
        "{\n"
        "  \"CELL\": [\n"
        "    { \"id\": \"Balmora\", \"old\": \"\", \"new\": \"SecondValue\", \"status\": \"translated\" }\n"
        "  ]\n"
        "}\n");

    DictMerger merger({ path1, path2 });
    const auto * entry = merger.getDict().at(tools_t::rec_type_t::CELL).find("Balmora");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->new_text == "FirstValue");

    removeTempFile(path1);
    removeTempFile(path2);
}

TEST_CASE("DictMerger key only in second dict", "[i]")
{
    const std::string path1 = "temp_merger_second_only1.json";
    const std::string path2 = "temp_merger_second_only2.json";

    tools_t::resetLog();

    writeTempDict(path1,
        "{\n"
        "  \"CELL\": [\n"
        "    { \"id\": \"Vivec\", \"old\": \"\", \"new\": \"VivecTrans\", \"status\": \"translated\" }\n"
        "  ]\n"
        "}\n");

    writeTempDict(path2,
        "{\n"
        "  \"CELL\": [\n"
        "    { \"id\": \"Balmora\", \"old\": \"\", \"new\": \"BalmoraTrans\", \"status\": \"translated\" }\n"
        "  ]\n"
        "}\n");

    DictMerger merger({ path1, path2 });
    const auto * entry = merger.getDict().at(tools_t::rec_type_t::CELL).find("Balmora");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->new_text == "BalmoraTrans");

    removeTempFile(path1);
    removeTempFile(path2);
}

TEST_CASE("DictMerger identical values", "[i]")
{
    const std::string path1 = "temp_merger_ident1.json";
    const std::string path2 = "temp_merger_ident2.json";

    tools_t::resetLog();

    writeTempDict(path1,
        "{\n"
        "  \"CELL\": [\n"
        "    { \"id\": \"Balmora\", \"old\": \"\", \"new\": \"Balmora\", \"status\": \"translated\" }\n"
        "  ]\n"
        "}\n");

    writeTempDict(path2,
        "{\n"
        "  \"CELL\": [\n"
        "    { \"id\": \"Balmora\", \"old\": \"\", \"new\": \"Balmora\", \"status\": \"translated\" }\n"
        "  ]\n"
        "}\n");

    DictMerger merger({ path1, path2 });
    const auto & chapter = merger.getDict().at(tools_t::rec_type_t::CELL);
    const auto * entry = chapter.find("Balmora");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->new_text == "Balmora");
    REQUIRE(chapter.size() == 1);

    removeTempFile(path1);
    removeTempFile(path2);
}

TEST_CASE("DictMerger duplicate values warning", "[i]")
{
    const std::string path1 = "temp_merger_dupval.json";

    tools_t::resetLog();

    writeTempDict(path1,
        "{\n"
        "  \"CELL\": [\n"
        "    { \"id\": \"Balmora, Guild of Fighters\", \"old\": \"\", \"new\": \"Balmora\", \"status\": \"translated\" },\n"
        "    { \"id\": \"Balmora, Guild of Mages\", \"old\": \"\", \"new\": \"Balmora\", \"status\": \"translated\" }\n"
        "  ]\n"
        "}\n");

    DictMerger merger({ path1 });

    std::string log = tools_t::getLog();
    REQUIRE(log.find("duplicate CELL value") != std::string::npos);
    REQUIRE(log.find("Balmora") != std::string::npos);

    removeTempFile(path1);
}
