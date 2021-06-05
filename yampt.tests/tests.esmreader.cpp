#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esmreader.hpp"

using namespace std;

TEST_CASE("loading and parsing plugin file", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    REQUIRE(esm.isLoaded() == true);
    REQUIRE(esm.getName().full == "Morrowind.esm");
    REQUIRE(esm.getName().prefix == "Morrowind");
    REQUIRE(esm.getName().suffix == ".esm");
}

TEST_CASE("set key, not found", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.selectRecord(0);
    esm.setKey("TEST");
    REQUIRE(esm.getKey().id == "TEST");
    REQUIRE(esm.getKey().exist == false);
    REQUIRE(esm.getKey().text == "N/A");
}

TEST_CASE("set key, empty text", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.selectRecord(16955); /* region CELL with empty NAME */
    esm.setKey("NAME");
    REQUIRE(esm.getKey().id == "NAME");
    REQUIRE(esm.getKey().exist == true);
    REQUIRE(esm.getKey().text == "");
}

TEST_CASE("set key", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.selectRecord(22075); /* regular CELL */
    esm.setKey("NAME");
    REQUIRE(esm.getKey().id == "NAME");
    REQUIRE(esm.getKey().exist == true);
    REQUIRE(esm.getKey().text == "Zergonipal, Shrine");
}

TEST_CASE("set key, INDX case", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.selectRecord(2062); /* SKIL with INDX instead of NAME */
    esm.setKey("INDX");
    REQUIRE(esm.getKey().id == "INDX");
    REQUIRE(esm.getKey().exist == true);
    REQUIRE(Tools::getINDX(esm.getKey().content) == "000");
}

TEST_CASE("set key, dialog type case", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.selectRecord(22245); /* DIAL with one byte DATA dialog type */
    esm.setKey("DATA");
    REQUIRE(esm.getKey().id == "DATA");
    REQUIRE(esm.getKey().exist == true);
    REQUIRE(Tools::getDialogType(esm.getKey().content) == "J");
}

TEST_CASE("set value, not found", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.selectRecord(0);
    esm.setValue("TEST");
    REQUIRE(esm.getValue().id == "TEST");
    REQUIRE(esm.getValue().exist == false);
    REQUIRE(esm.getValue().text == "N/A");
    REQUIRE(esm.getValue().pos == esm.getRecordContent().size());
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

    EsmReader esm("master/en/Morrowind.esm");
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

    EsmReader esm("master/en/Morrowind.esm");
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
