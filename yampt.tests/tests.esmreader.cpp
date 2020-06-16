#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esmreader.hpp"

using namespace std;

TEST_CASE("Loading and parsing plugin file", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    REQUIRE(esm.isLoaded() == true);
    REQUIRE(esm.getNameFull() == "Morrowind.esm");
    REQUIRE(esm.getNamePrefix() == "Morrowind");
    REQUIRE(esm.getNameSuffix() == ".esm");
}

TEST_CASE("Set unique, default case, not found", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.setRecord(0);
    esm.setKey("TEST");
    REQUIRE(esm.getUniqueId() == "TEST");
    REQUIRE(esm.isUniqueValid() == false);
    REQUIRE(esm.getUniqueText() == "Unique id TEST not found!");
}

TEST_CASE("Set unique, default case, empty text", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.setRecord(16955); // Region CELL with empty NAME
    esm.setKey("NAME");
    REQUIRE(esm.getUniqueId() == "NAME");
    REQUIRE(esm.isUniqueValid() == true);
    REQUIRE(esm.getUniqueText() == "");
}

TEST_CASE("Set unique, default case", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.setRecord(22075); // Regular CELL
    esm.setKey("NAME");
    REQUIRE(esm.getUniqueId() == "NAME");
    REQUIRE(esm.isUniqueValid() == true);
    REQUIRE(esm.getUniqueText() == "Zergonipal, Shrine");
}

TEST_CASE("Set unique, INDX case", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.setRecord(2062); // SKIL with INDX instead of NAME
    esm.setKey("INDX");
    REQUIRE(esm.getUniqueId() == "INDX");
    REQUIRE(esm.isUniqueValid() == true);
    REQUIRE(esm.getUniqueText() == "000");
}

TEST_CASE("Set unique, dialog type case", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.setRecord(22245); // DIAL with one byte DATA dialog type
    esm.setKey("DATA");
    REQUIRE(esm.getUniqueId() == "DATA");
    REQUIRE(esm.isUniqueValid() == true);
    REQUIRE(esm.getUniqueText() == "J");
}

TEST_CASE("Set friendly, not found", "[i]")
{
    EsmReader esm("master/en/Morrowind.esm");
    esm.setRecord(0);
    esm.setValue("TEST");
    REQUIRE(esm.getFriendlyId() == "TEST");
    REQUIRE(esm.isFriendlyValid() == false);
    REQUIRE(esm.getFriendlyText() == "Friendly id TEST not found!");
    REQUIRE(esm.getFriendlyPos() == esm.getRecordContent().size());
    REQUIRE(esm.getFriendlySize() == 0);
}

TEST_CASE("Set friendly", "[i]")
{
    /*
       (16) FACT............
       (16) NAME....Redoran.
       (28) FNAM....Great House Redoran.
   (pos) -> RNAM....Hireling........................
                    ^            size              ^
    */

    EsmReader esm("master/en/Morrowind.esm");
    esm.setRecord(1600); // RNAM
    esm.setValue("RNAM");
    REQUIRE(esm.getFriendlyId() == "RNAM");
    REQUIRE(esm.isFriendlyValid() == true);
    REQUIRE(esm.getFriendlyText() == "Hireling");
    REQUIRE(esm.getFriendlyWithNull().size() == 32);
    REQUIRE(esm.getFriendlyPos() == (16 + 16 + 28));
    REQUIRE(esm.getFriendlySize() == 32);
}

TEST_CASE("Set friendly, next", "[i]")
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
    esm.setRecord(1600); // RNAM
    esm.setValue("RNAM");
    esm.setNextValue("RNAM");
    REQUIRE(esm.getFriendlyId() == "RNAM");
    REQUIRE(esm.isFriendlyValid() == true);
    REQUIRE(esm.getFriendlyWithNull().size() == 32);
    REQUIRE(esm.getFriendlyText() == "Retainer");
    REQUIRE(esm.getFriendlyPos() == (16 + 16 + 28 + 40));
    REQUIRE(esm.getFriendlySize() == 32);
}
