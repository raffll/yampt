#include <catch2/catch.hpp>
#include "../src/config.hpp"
#include "../src/esmreader.hpp"
#include "../src/dictcreator.hpp"

using namespace std;

TEST_CASE("Create raw")
{
    DictCreator esm("master/en/Morrowind.esm");
    REQUIRE(esm.getDict(yampt::rec_type::CELL).begin()->first == "Abaelun Mine");
    REQUIRE(esm.getDict(yampt::rec_type::CELL).begin()->second == "Abaelun Mine");
    REQUIRE(esm.getDict(yampt::rec_type::DIAL).begin()->first == "1000-drake pledge");
    REQUIRE(esm.getDict(yampt::rec_type::DIAL).begin()->second == "1000-drake pledge");
    REQUIRE(esm.getDict(yampt::rec_type::INDX).begin()->first == "MGEF^000");
    REQUIRE(esm.getDict(yampt::rec_type::INDX).begin()->second == "This effect permits the subject to breathe underwater for the duration.");
    REQUIRE(esm.getDict(yampt::rec_type::RNAM).begin()->first == "Ashlanders^0");
    REQUIRE(esm.getDict(yampt::rec_type::RNAM).begin()->second == "Clanfriend");
    REQUIRE(esm.getDict(yampt::rec_type::DESC).begin()->first == "BSGN^Beggar's Nose");
    REQUIRE(esm.getDict(yampt::rec_type::DESC).begin()->second == "Constellation of The Tower with a Prime Aspect of Secunda.");
    REQUIRE(esm.getDict(yampt::rec_type::GMST).begin()->first == "s3dAudio");
    REQUIRE(esm.getDict(yampt::rec_type::GMST).begin()->second == "3D Audio");
    REQUIRE(esm.getDict(yampt::rec_type::FNAM).begin()->first == "ACTI^A_Ex_De_Oar");
    REQUIRE(esm.getDict(yampt::rec_type::FNAM).begin()->second == "Oar");
    REQUIRE(esm.getDict(yampt::rec_type::INFO).begin()->first == "G^Greeting 0^1046442603030210731");
    REQUIRE(esm.getDict(yampt::rec_type::INFO).begin()->second == "I've heard you've got a price on your head, %PCRank. For a small fee, I can take care of that.");
    REQUIRE(esm.getDict(yampt::rec_type::TEXT).begin()->first == "BookSkill_Acrobatics2");
    //REQUIRE(esm.getDict(yampt::rec_type::TEXT).begin()->second.size() == 8459);
    REQUIRE(esm.getDict(yampt::rec_type::BNAM).begin()->first == esm.getDict(yampt::rec_type::BNAM).begin()->second);
    REQUIRE(esm.getDict(yampt::rec_type::SCTX).begin()->first == esm.getDict(yampt::rec_type::SCTX).begin()->second);
}

TEST_CASE("Create base, same record order")
{
    DictCreator esm("master/pl/Morrowind.esm", "master/en/Morrowind.esm");
    auto search = esm.getDict(yampt::rec_type::CELL).find("Abaelun Mine");
    REQUIRE(search != esm.getDict(yampt::rec_type::CELL).end());
    REQUIRE(search->second == "Kopalnia Abaelun");
    search = esm.getDict(yampt::rec_type::DIAL).find("Abebaal Egg Mine");
    REQUIRE(search != esm.getDict(yampt::rec_type::DIAL).end());
    REQUIRE(search->second == "kopalnia jaj Abebaal");
    search = esm.getDict(yampt::rec_type::INFO).find("T^kopalnia jaj Abebaal^3253555431180022526");
    REQUIRE(search != esm.getDict(yampt::rec_type::INFO).end());
    search = esm.getDict(yampt::rec_type::INFO).find("T^Abebaal Egg Mine^3253555431180022526");
    REQUIRE(search == esm.getDict(yampt::rec_type::INFO).end());
}

TEST_CASE("Create base, different record order")
{
    DictCreator esm("master/de/Morrowind.esm", "master/en/Morrowind.esm");
    auto search = esm.getDict(yampt::rec_type::CELL).find("Abaelun Mine");
    REQUIRE(search != esm.getDict(yampt::rec_type::CELL).end());
    REQUIRE(search->second == "Abaelun-Mine");
    search = esm.getDict(yampt::rec_type::DIAL).find("Abebaal Egg Mine");
    REQUIRE(search != esm.getDict(yampt::rec_type::DIAL).end());
    REQUIRE(search->second == "Eiermine Abebaal");
}
