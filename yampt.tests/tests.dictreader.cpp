#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dictreader.hpp"

#include <fstream>
#include <cstdio>

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

TEST_CASE("parse dict", "[i]")
{
    const string path = "temp_test_dict.xml";
    const string content =
        "<record>\r\n"
        "\t<_id>CELL</_id>\r\n"
        "\t<key>Balmora</key>\r\n"
        "\t<val>Balmora</val>\r\n"
        "</record>\r\n";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);
    REQUIRE(reader.getDict().at(Tools::RecType::CELL).count("Balmora") == 1);
    REQUIRE(reader.getDict().at(Tools::RecType::CELL).at("Balmora") == "Balmora");

    removeTempFile(path);
}

TEST_CASE("DictReader rejects duplicate key", "[i]")
{
    const string path = "temp_test_dict_dup.xml";
    const string content =
        "<record>\r\n"
        "\t<_id>CELL</_id>\r\n"
        "\t<key>Vivec</key>\r\n"
        "\t<val>Vivec_First</val>\r\n"
        "</record>\r\n"
        "<record>\r\n"
        "\t<_id>CELL</_id>\r\n"
        "\t<key>Vivec</key>\r\n"
        "\t<val>Vivec_Second</val>\r\n"
        "</record>\r\n";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);
    REQUIRE(reader.getDict().at(Tools::RecType::CELL).count("Vivec") == 1);

    removeTempFile(path);
}

TEST_CASE("DictReader rejects unknown type string", "[i]")
{
    const string path = "temp_test_dict_bogus.xml";
    const string content =
        "<record>\r\n"
        "\t<_id>BOGUS</_id>\r\n"
        "\t<key>SomeKey</key>\r\n"
        "\t<val>SomeVal</val>\r\n"
        "</record>\r\n";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.isLoaded() == true);
    REQUIRE(Tools::getNumberOfElementsInDict(reader.getDict()) == 0);

    removeTempFile(path);
}

TEST_CASE("DictReader rejects CELL value > 63 bytes", "[i]")
{
    const string path = "temp_test_cell_long.xml";
    const string val64(64, 'A');
    const string content =
        "<record>\r\n"
        "\t<_id>CELL</_id>\r\n"
        "\t<key>TestCell</key>\r\n"
        "\t<val>" + val64 + "</val>\r\n"
        "</record>\r\n";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.getDict().at(Tools::RecType::CELL).count("TestCell") == 0);

    removeTempFile(path);
}

TEST_CASE("DictReader rejects RNAM value > 32 bytes", "[i]")
{
    const string path = "temp_test_rnam_long.xml";
    const string val33(33, 'B');
    const string content =
        "<record>\r\n"
        "\t<_id>RNAM</_id>\r\n"
        "\t<key>TestRnam</key>\r\n"
        "\t<val>" + val33 + "</val>\r\n"
        "</record>\r\n";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.getDict().at(Tools::RecType::RNAM).count("TestRnam") == 0);

    removeTempFile(path);
}

TEST_CASE("DictReader rejects FNAM value > 31 bytes", "[i]")
{
    const string path = "temp_test_fnam_long.xml";
    const string val32(32, 'C');
    const string content =
        "<record>\r\n"
        "\t<_id>FNAM</_id>\r\n"
        "\t<key>TestFnam</key>\r\n"
        "\t<val>" + val32 + "</val>\r\n"
        "</record>\r\n";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.getDict().at(Tools::RecType::FNAM).count("TestFnam") == 0);

    removeTempFile(path);
}

TEST_CASE("DictReader rejects INFO value > 1024 bytes", "[i]")
{
    const string path = "temp_test_info_long.xml";
    const string val1025(1025, 'D');
    const string content =
        "<record>\r\n"
        "\t<_id>INFO</_id>\r\n"
        "\t<key>TestInfo</key>\r\n"
        "\t<val>" + val1025 + "</val>\r\n"
        "</record>\r\n";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.getDict().at(Tools::RecType::INFO).count("TestInfo") == 0);

    removeTempFile(path);
}

TEST_CASE("DictReader accepts INFO value 513-1024 bytes", "[i]")
{
    const string path = "temp_test_info_mid.xml";
    const string val513(513, 'E');
    const string content =
        "<record>\r\n"
        "\t<_id>INFO</_id>\r\n"
        "\t<key>TestInfoMid</key>\r\n"
        "\t<val>" + val513 + "</val>\r\n"
        "</record>\r\n";

    writeTempDict(path, content);

    DictReader reader(path);

    REQUIRE(reader.getDict().at(Tools::RecType::INFO).count("TestInfoMid") == 1);

    removeTempFile(path);
}
