#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esmreader.hpp"
#include "../yampt/dictcreator.hpp"

#include <string>

extern std::string g_masterPath;

TEST_CASE("make without base dict produces untranslated status", "[i]")
{
    DictCreator esm(g_masterPath + "en/Morrowind.esm");
    const auto & dict = esm.getDict();

    const auto * cell = dict.at(Tools::RecType::CELL).find("Abaelun Mine");
    REQUIRE(cell != nullptr);
    REQUIRE(cell->original == "Abaelun Mine");
    REQUIRE(cell->translation == "");
    REQUIRE(cell->status == Tools::Status::untranslated);

    const auto * dial = dict.at(Tools::RecType::DIAL).find("1000-drake pledge");
    REQUIRE(dial != nullptr);
    REQUIRE(dial->original == "1000-drake pledge");
    REQUIRE(dial->translation == "");
    REQUIRE(dial->status == Tools::Status::untranslated);

    const auto * indx = dict.at(Tools::RecType::INDX).find("MGEF^000");
    REQUIRE(indx != nullptr);
    REQUIRE(indx->original == "This effect permits the subject to breathe underwater for the duration.");
    REQUIRE(indx->translation == "");
    REQUIRE(indx->status == Tools::Status::untranslated);

    const auto * rnam = dict.at(Tools::RecType::RNAM).find("Ashlanders^0");
    REQUIRE(rnam != nullptr);
    REQUIRE(rnam->original == "Clanfriend");
    REQUIRE(rnam->translation == "");
    REQUIRE(rnam->status == Tools::Status::untranslated);

    const auto * desc = dict.at(Tools::RecType::DESC).find("BSGN^Beggar's Nose");
    REQUIRE(desc != nullptr);
    REQUIRE(desc->original == "Constellation of The Tower with a Prime Aspect of Secunda.");
    REQUIRE(desc->translation == "");
    REQUIRE(desc->status == Tools::Status::untranslated);

    const auto * gmst = dict.at(Tools::RecType::GMST).find("s3dAudio");
    REQUIRE(gmst != nullptr);
    REQUIRE(gmst->original == "3D Audio");
    REQUIRE(gmst->translation == "");
    REQUIRE(gmst->status == Tools::Status::untranslated);

    const auto * fnam = dict.at(Tools::RecType::FNAM).find("ACTI^A_Ex_De_Oar");
    REQUIRE(fnam != nullptr);
    REQUIRE(fnam->original == "Oar");
    REQUIRE(fnam->translation == "");
    REQUIRE(fnam->status == Tools::Status::untranslated);

    const auto * info = dict.at(Tools::RecType::INFO).find("G^Greeting 0^1046442603030210731");
    REQUIRE(info != nullptr);
    REQUIRE(info->original == "I've heard you've got a price on your head, %PCRank. For a small fee, I can take care of that.");
    REQUIRE(info->translation == "");
    REQUIRE(info->status == Tools::Status::untranslated);

    const auto * text = dict.at(Tools::RecType::TEXT).find("BookSkill_Acrobatics2");
    REQUIRE(text != nullptr);
    REQUIRE(text->original.size() == 10305);
    REQUIRE(text->translation == "");
    REQUIRE(text->status == Tools::Status::untranslated);
}

TEST_CASE("make-base, same record order", "[i]")
{
    DictCreator esm(g_masterPath + "pl/Morrowind.esm", g_masterPath + "en/Morrowind.esm");
    const auto & dict = esm.getDict();

    const auto * cell = dict.at(Tools::RecType::CELL).find("Abaelun Mine");
    REQUIRE(cell != nullptr);
    REQUIRE(cell->translation == "Kopalnia Abaelun");

    const auto * dial = dict.at(Tools::RecType::DIAL).find("Abebaal Egg Mine");
    REQUIRE(dial != nullptr);
    REQUIRE(dial->translation == "kopalnia jaj Abebaal");

    const auto * info = dict.at(Tools::RecType::INFO).find("T^kopalnia jaj Abebaal^3253555431180022526");
    REQUIRE(info != nullptr);

    const auto * info_en = dict.at(Tools::RecType::INFO).find("T^Abebaal Egg Mine^3253555431180022526");
    REQUIRE(info_en == nullptr);
}

TEST_CASE("make-base, different record order", "[i]")
{
    DictCreator esm(g_masterPath + "de/Morrowind.esm", g_masterPath + "en/Morrowind.esm");
    const auto & dict = esm.getDict();

    const auto * cell = dict.at(Tools::RecType::CELL).find("Abaelun Mine");
    REQUIRE(cell != nullptr);
    REQUIRE(cell->translation == "Abaelun-Mine");

    const auto * cell_missing = dict.at(Tools::RecType::CELL).find("Aharunartus");
    REQUIRE(cell_missing != nullptr);
    REQUIRE(cell_missing->translation == "MISSING");

    const auto * dial = dict.at(Tools::RecType::DIAL).find("Abebaal Egg Mine");
    REQUIRE(dial != nullptr);
    REQUIRE(dial->translation == "Eiermine Abebaal");
}
