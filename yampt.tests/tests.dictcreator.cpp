#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esmreader.hpp"
#include "../yampt/dictcreator.hpp"

TEST_CASE("create raw", "[i]")
{
    DictCreator esm("master/en/Morrowind.esm");
    REQUIRE(esm.getDict().at(Tools::RecType::CELL).begin()->first == "Abaelun Mine");
    REQUIRE(esm.getDict().at(Tools::RecType::CELL).begin()->second == "Abaelun Mine");
    REQUIRE(esm.getDict().at(Tools::RecType::DIAL).begin()->first == "1000-drake pledge");
    REQUIRE(esm.getDict().at(Tools::RecType::DIAL).begin()->second == "1000-drake pledge");
    REQUIRE(esm.getDict().at(Tools::RecType::INDX).begin()->first == "MGEF^000");
    REQUIRE(esm.getDict().at(Tools::RecType::INDX).begin()->second == "This effect permits the subject to breathe underwater for the duration.");
    REQUIRE(esm.getDict().at(Tools::RecType::RNAM).begin()->first == "Ashlanders^0");
    REQUIRE(esm.getDict().at(Tools::RecType::RNAM).begin()->second == "Clanfriend");
    REQUIRE(esm.getDict().at(Tools::RecType::DESC).begin()->first == "BSGN^Beggar's Nose");
    REQUIRE(esm.getDict().at(Tools::RecType::DESC).begin()->second == "Constellation of The Tower with a Prime Aspect of Secunda.");
    REQUIRE(esm.getDict().at(Tools::RecType::GMST).begin()->first == "s3dAudio");
    REQUIRE(esm.getDict().at(Tools::RecType::GMST).begin()->second == "3D Audio");
    REQUIRE(esm.getDict().at(Tools::RecType::FNAM).begin()->first == "ACTI^A_Ex_De_Oar");
    REQUIRE(esm.getDict().at(Tools::RecType::FNAM).begin()->second == "Oar");
    REQUIRE(esm.getDict().at(Tools::RecType::INFO).begin()->first == "G^Greeting 0^1046442603030210731");
    REQUIRE(esm.getDict().at(Tools::RecType::INFO).begin()->second == "I've heard you've got a price on your head, %PCRank. For a small fee, I can take care of that.");
    REQUIRE(esm.getDict().at(Tools::RecType::TEXT).begin()->first == "BookSkill_Acrobatics2");
    REQUIRE(esm.getDict().at(Tools::RecType::TEXT).begin()->second.size() == 10305);
    REQUIRE(esm.getDict().at(Tools::RecType::BNAM).begin()->first == esm.getDict().at(Tools::RecType::BNAM).begin()->second);
    REQUIRE(esm.getDict().at(Tools::RecType::SCTX).begin()->first == esm.getDict().at(Tools::RecType::SCTX).begin()->second);
}

TEST_CASE("create base, same record order", "[i]")
{
    DictCreator esm("master/pl/Morrowind.esm", "master/en/Morrowind.esm");
    auto search = esm.getDict().at(Tools::RecType::CELL).find("Abaelun Mine");
    REQUIRE(search != esm.getDict().at(Tools::RecType::CELL).end());
    REQUIRE(search->second == "Kopalnia Abaelun");

    search = esm.getDict().at(Tools::RecType::DIAL).find("Abebaal Egg Mine");
    REQUIRE(search != esm.getDict().at(Tools::RecType::DIAL).end());
    REQUIRE(search->second == "kopalnia jaj Abebaal");

    search = esm.getDict().at(Tools::RecType::INFO).find("T^kopalnia jaj Abebaal^3253555431180022526");
    REQUIRE(search != esm.getDict().at(Tools::RecType::INFO).end());

    search = esm.getDict().at(Tools::RecType::INFO).find("T^Abebaal Egg Mine^3253555431180022526");
    REQUIRE(search == esm.getDict().at(Tools::RecType::INFO).end());
}

TEST_CASE("create base, different record order", "[i]")
{
    DictCreator esm("master/de/Morrowind.esm", "master/en/Morrowind.esm");

    auto search = esm.getDict().at(Tools::RecType::CELL).find("Abaelun Mine");
    REQUIRE(search != esm.getDict().at(Tools::RecType::CELL).end());
    REQUIRE(search->second == "Abaelun-Mine");

    search = esm.getDict().at(Tools::RecType::CELL).find("Aharunartus");
    REQUIRE(search != esm.getDict().at(Tools::RecType::CELL).end());
    REQUIRE(search->second == "<err name=\"MISSING\"/>");

    search = esm.getDict().at(Tools::RecType::DIAL).find("Abebaal Egg Mine");
    REQUIRE(search != esm.getDict().at(Tools::RecType::DIAL).end());
    REQUIRE(search->second == "Eiermine Abebaal");
}
