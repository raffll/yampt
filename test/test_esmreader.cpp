#include <catch2/catch.hpp>
#include "../src/config.hpp"
#include "../src/esmreader.hpp"

using namespace std;

static EsmReader esm("Morrowind.esm");

TEST_CASE("loading Morrowind.esm")
{
    REQUIRE(esm.getIsLoaded() == true);
    REQUIRE(esm.getNameFull() == "Morrowind.esm");
    REQUIRE(esm.getNamePrefix() == "Morrowind");
    REQUIRE(esm.getNameSuffix() == ".esm");
}

TEST_CASE("default case in set unique not found")
{
    esm.setRecordTo(0);
    esm.setUniqueTo("TEST");
    REQUIRE(esm.getUniqueId() == "TEST");
    REQUIRE(esm.getUniqueStatus() == false);
    REQUIRE(esm.getUniqueText() == "Unique id TEST not found!");
}

TEST_CASE("default case in set unique with empty subrecord")
{
    esm.setRecordTo(16955); // Region CELL with empty NAME
    esm.setUniqueTo("NAME");
    REQUIRE(esm.getUniqueId() == "NAME");
    REQUIRE(esm.getUniqueStatus() == false);
    REQUIRE(esm.getUniqueText() == "Unique text is empty!");
}

TEST_CASE("default case in set unique")
{
    esm.setRecordTo(22075); // Regular CELL
    esm.setUniqueTo("NAME");
    REQUIRE(esm.getUniqueId() == "NAME");
    REQUIRE(esm.getUniqueStatus() == true);
    REQUIRE(esm.getUniqueText() == "Zergonipal, Shrine");
}

TEST_CASE("INDX case in set unique")
{
    esm.setRecordTo(2062); // SKIL with INDX instead of NAME
    esm.setUniqueTo("INDX");
    REQUIRE(esm.getUniqueId() == "INDX");
    REQUIRE(esm.getUniqueStatus() == true);
    REQUIRE(esm.getUniqueText() == "000");
}

TEST_CASE("dialog type case in set unique")
{
    esm.setRecordTo(22245); // DIAL with one byte DATA dialog type
    esm.setUniqueTo("DATA");
    REQUIRE(esm.getUniqueId() == "DATA");
    REQUIRE(esm.getUniqueStatus() == true);
    REQUIRE(esm.getUniqueText() == "J");
}

TEST_CASE("set first friendly not found")
{
    esm.setRecordTo(0);
    esm.setFirstFriendlyTo("TEST");
    REQUIRE(esm.getFriendlyId() == "TEST");
    REQUIRE(esm.getFriendlyStatus() == false);
    REQUIRE(esm.getFriendlyText() == "Friendly id TEST not found!");
    REQUIRE(esm.getFriendlyPos() == esm.getRecordContent().size());
    REQUIRE(esm.getFriendlySize() == 0);
}

TEST_CASE("set first friendly")
{
    /*
       (16) FACT............
       (16) NAME....Redoran.
       (28) FNAM....Great House Redoran.
   (pos) -> RNAM....Hireling........................
                    ^            size              ^
    */

    esm.setRecordTo(1600); // RNAM
    esm.setFirstFriendlyTo("RNAM");
    REQUIRE(esm.getFriendlyId() == "RNAM");
    REQUIRE(esm.getFriendlyStatus() == true);
    REQUIRE(esm.getFriendlyText() == "Hireling");
    REQUIRE(esm.getFriendlyPos() == (16 + 16 + 28));
    REQUIRE(esm.getFriendlySize() == 32);
}

TEST_CASE("set next friendly")
{
    /*
       (16) FACT............
       (16) NAME....Redoran.
       (28) FNAM....Great House Redoran.
       (40) RNAM....Hireling........................
   (pos) -> RNAM....Retainer........................
                    ^            size              ^
    */

    esm.setRecordTo(1600); // RNAM
    esm.setFirstFriendlyTo("RNAM");
    esm.setNextFriendlyTo("RNAM");
    REQUIRE(esm.getFriendlyId() == "RNAM");
    REQUIRE(esm.getFriendlyStatus() == true);
    REQUIRE(esm.getFriendlyText() == "Retainer");
    REQUIRE(esm.getFriendlyPos() == (16 + 16 + 28 + 40));
    REQUIRE(esm.getFriendlySize() == 32);
}

