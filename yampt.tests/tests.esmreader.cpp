#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esmreader.hpp"

#include <string>

extern std::string g_masterPath;

using namespace std;

TEST_CASE("loading and parsing plugin file", "[i]")
{
    EsmReader esm((g_masterPath + "en/Morrowind.esm"));
    REQUIRE(esm.isLoaded() == true);
    REQUIRE(esm.getName().full == "Morrowind.esm");
    REQUIRE(esm.getName().name == "Morrowind");
    REQUIRE(esm.getName().ext == ".esm");
}

TEST_CASE("set key, not found", "[i]")
{
    EsmReader esm((g_masterPath + "en/Morrowind.esm"));
    esm.selectRecord(0);
    esm.setKey("TEST");
    REQUIRE(esm.getKey().id == "TEST");
    REQUIRE(esm.getKey().exist == false);
    REQUIRE(esm.getKey().text == "N/A");
}

TEST_CASE("set key, empty text", "[i]")
{
    EsmReader esm((g_masterPath + "en/Morrowind.esm"));
    esm.selectRecord(16955); /* region CELL with empty NAME */
    esm.setKey("NAME");
    REQUIRE(esm.getKey().id == "NAME");
    REQUIRE(esm.getKey().exist == true);
    REQUIRE(esm.getKey().text == "");
}

TEST_CASE("set key", "[i]")
{
    EsmReader esm((g_masterPath + "en/Morrowind.esm"));
    esm.selectRecord(22075); /* regular CELL */
    esm.setKey("NAME");
    REQUIRE(esm.getKey().id == "NAME");
    REQUIRE(esm.getKey().exist == true);
    REQUIRE(esm.getKey().text == "Zergonipal, Shrine");
}

TEST_CASE("set key, INDX case", "[i]")
{
    EsmReader esm((g_masterPath + "en/Morrowind.esm"));
    esm.selectRecord(2062); /* SKIL with INDX instead of NAME */
    esm.setKey("INDX");
    REQUIRE(esm.getKey().id == "INDX");
    REQUIRE(esm.getKey().exist == true);
    REQUIRE(tools_t::getINDX(esm.getKey().content) == "000");
}

TEST_CASE("set key, dialog type case", "[i]")
{
    EsmReader esm((g_masterPath + "en/Morrowind.esm"));
    esm.selectRecord(22245); /* DIAL with one byte DATA dialog type */
    esm.setKey("DATA");
    REQUIRE(esm.getKey().id == "DATA");
    REQUIRE(esm.getKey().exist == true);
    REQUIRE(tools_t::getDialogType(esm.getKey().content) == "J");
}

TEST_CASE("set value, not found", "[i]")
{
    EsmReader esm((g_masterPath + "en/Morrowind.esm"));
    esm.selectRecord(0);
    esm.setValue("TEST");
    REQUIRE(esm.getValue().id == "TEST");
    REQUIRE(esm.getValue().exist == false);
    REQUIRE(esm.getValue().text == "N/A");
    REQUIRE(esm.getValue().pos == esm.getRecord().content.size());
    REQUIRE(esm.getValue().size == 0);
}

TEST_CASE("set value", "[i]")
{
    /*
       (16) FACT............
       (16) NAME....Redoran.
       (28) FNAM....Great House Redoran.
   (pos) -> RNAM....Hireling........................
                    ^            size              ^
    */

    EsmReader esm((g_masterPath + "en/Morrowind.esm"));
    esm.selectRecord(1600); /* RNAM */
    esm.setValue("RNAM");
    REQUIRE(esm.getValue().id == "RNAM");
    REQUIRE(esm.getValue().exist == true);
    REQUIRE(esm.getValue().text == "Hireling");
    REQUIRE(esm.getValue().content.size() == 32);
    REQUIRE(esm.getValue().pos == (16 + 16 + 28));
    REQUIRE(esm.getValue().size == 32);
}

TEST_CASE("set value, next", "[i]")
{
    /*
       (16) FACT............
       (16) NAME....Redoran.
       (28) FNAM....Great House Redoran.
       (40) RNAM....Hireling........................
   (pos) -> RNAM....Retainer........................
                    ^            size              ^
    */

    EsmReader esm((g_masterPath + "en/Morrowind.esm"));
    esm.selectRecord(1600); /* RNAM */
    esm.setValue("RNAM");
    esm.setNextValue("RNAM");
    REQUIRE(esm.getValue().id == "RNAM");
    REQUIRE(esm.getValue().exist == true);
    REQUIRE(esm.getValue().content.size() == 32);
    REQUIRE(esm.getValue().text == "Retainer");
    REQUIRE(esm.getValue().pos == (16 + 16 + 28 + 40));
    REQUIRE(esm.getValue().size == 32);
}

TEST_CASE("setModified increments count", "[i]")
{
    EsmReader esm((g_masterPath + "en/Morrowind.esm"));
    REQUIRE(esm.isLoaded() == true);
    size_t before = esm.getModifiedCount();
    esm.setModified(0);
    REQUIRE(esm.getModifiedCount() == before + 1);
}

TEST_CASE("loading non-existent file", "[i]")
{
    EsmReader esm("this/path/does/not/exist.esm");
    REQUIRE(esm.isLoaded() == false);
}

TEST_CASE("loading non-TES3 file", "[i]")
{
    const std::string temp_path = "temp_not_tes3.esm";
    {
        std::ofstream f(temp_path, std::ios::binary);
        f << "XXXX\x00\x00\x00\x00not a tes3 file";
    }
    EsmReader esm(temp_path);
    REQUIRE(esm.isLoaded() == false);
    std::remove(temp_path.c_str());
}

TEST_CASE("splitFile bounds check on corrupt record size", "[u]")
{
    const std::string temp_path = "temp_corrupt_size.esm";
    {
        std::ofstream f(temp_path, std::ios::binary);
        // Valid TES3 header record: "TES3" + size (4 bytes LE) + 8 padding bytes + 4 bytes data
        // Record size field = total_record_size - 16, so for a 20-byte record: size field = 4
        std::string header = "TES3";
        header += std::string("\x04\x00\x00\x00", 4); // size = 4 (record is 20 bytes total)
        header += std::string(8, '\0');                // 8 bytes flags/padding
        header += std::string(4, '\0');                // 4 bytes of data
        // Second record with garbage size that exceeds file
        std::string corrupt = "GLOB";
        corrupt += std::string("\xFF\xFF\x00\x00", 4); // size = 65535, way beyond file
        corrupt += std::string(8, '\0');                // 8 bytes flags/padding
        f << header << corrupt;
    }
    EsmReader esm(temp_path);
    REQUIRE(esm.isLoaded() == true);
    REQUIRE(esm.getRecords().size() == 1); // only the first valid record retained
    std::remove(temp_path.c_str());
}
