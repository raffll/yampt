#include <catch2/catch.hpp>
#include "../src/config.hpp"
#include "../src/esmreader.hpp"

using namespace std;

static EsmReader esm("Morrowind.esm");

TEST_CASE("Loading Morrowind.esm")
{
    REQUIRE(esm.isLoaded() == true);
    REQUIRE(esm.getNameFull() == "Morrowind.esm");
    REQUIRE(esm.getNamePrefix() == "Morrowind");
    REQUIRE(esm.getNameSuffix() == ".esm");
}

TEST_CASE("Default case in set unique when not found")
{
    esm.setRecordTo(0);
    esm.setUniqueTo("TEST");
    REQUIRE(esm.getUniqueId() == "TEST");
    REQUIRE(esm.isUniqueValid() == false);
    REQUIRE(esm.getUniqueText() == "Unique id TEST not found!");
}

TEST_CASE("CELL case in set unique with empty subrecord")
{
    esm.setRecordTo(16955); // Region CELL with empty NAME
    esm.setUniqueTo("NAME");
    REQUIRE(esm.getUniqueId() == "NAME");
    REQUIRE(esm.isUniqueValid() == true);
    REQUIRE(esm.getUniqueText() == "");
}

TEST_CASE("Default case in set unique")
{
    esm.setRecordTo(22075); // Regular CELL
    esm.setUniqueTo("NAME");
    REQUIRE(esm.getUniqueId() == "NAME");
    REQUIRE(esm.isUniqueValid() == true);
    REQUIRE(esm.getUniqueText() == "Zergonipal, Shrine");
}

TEST_CASE("INDX case in set unique")
{
    esm.setRecordTo(2062); // SKIL with INDX instead of NAME
    esm.setUniqueTo("INDX");
    REQUIRE(esm.getUniqueId() == "INDX");
    REQUIRE(esm.isUniqueValid() == true);
    REQUIRE(esm.getUniqueText() == "000");
}

TEST_CASE("Dialog type case in set unique")
{
    esm.setRecordTo(22245); // DIAL with one byte DATA dialog type
    esm.setUniqueTo("DATA");
    REQUIRE(esm.getUniqueId() == "DATA");
    REQUIRE(esm.isUniqueValid() == true);
    REQUIRE(esm.getUniqueText() == "J");
}

TEST_CASE("Set first friendly when not found")
{
    esm.setRecordTo(0);
    esm.setFriendlyTo("TEST");
    REQUIRE(esm.getFriendlyId() == "TEST");
    REQUIRE(esm.isFriendlyValid() == false);
    REQUIRE(esm.getFriendlyText() == "Friendly id TEST not found!");
    REQUIRE(esm.getFriendlyPos() == esm.getRecordContent().size());
    REQUIRE(esm.getFriendlySize() == 0);
}

TEST_CASE("Set first friendly")
{
    /*
       (16) FACT............
       (16) NAME....Redoran.
       (28) FNAM....Great House Redoran.
   (pos) -> RNAM....Hireling........................
                    ^            size              ^
    */

    esm.setRecordTo(1600); // RNAM
    esm.setFriendlyTo("RNAM");
    REQUIRE(esm.getFriendlyId() == "RNAM");
    REQUIRE(esm.isFriendlyValid() == true);
    REQUIRE(esm.getFriendlyText() == "Hireling");
    REQUIRE(esm.getFriendlyPos() == (16 + 16 + 28));
    REQUIRE(esm.getFriendlySize() == 32);
}

TEST_CASE("Set next friendly")
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
    esm.setFriendlyTo("RNAM");
    esm.setNextFriendlyTo("RNAM");
    REQUIRE(esm.getFriendlyId() == "RNAM");
    REQUIRE(esm.isFriendlyValid() == true);
    REQUIRE(esm.getFriendlyText() == "Retainer");
    REQUIRE(esm.getFriendlyPos() == (16 + 16 + 28 + 40));
    REQUIRE(esm.getFriendlySize() == 32);
}
