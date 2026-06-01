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
    REQUIRE(cell->old_text == "Abaelun Mine");
    REQUIRE(cell->new_text == "");
    REQUIRE(cell->status == Tools::Status::untranslated);

    const auto * dial = dict.at(Tools::RecType::DIAL).find("1000-drake pledge");
    REQUIRE(dial != nullptr);
    REQUIRE(dial->old_text == "1000-drake pledge");
    REQUIRE(dial->new_text == "");
    REQUIRE(dial->status == Tools::Status::untranslated);

    const auto * indx = dict.at(Tools::RecType::INDX).find("MGEF^000");
    REQUIRE(indx != nullptr);
    REQUIRE(indx->old_text == "This effect permits the subject to breathe underwater for the duration.");
    REQUIRE(indx->new_text == "");
    REQUIRE(indx->status == Tools::Status::untranslated);

    const auto * rnam = dict.at(Tools::RecType::RNAM).find("Ashlanders^0");
    REQUIRE(rnam != nullptr);
    REQUIRE(rnam->old_text == "Clanfriend");
    REQUIRE(rnam->new_text == "");
    REQUIRE(rnam->status == Tools::Status::untranslated);

    const auto * desc = dict.at(Tools::RecType::DESC).find("BSGN^Beggar's Nose");
    REQUIRE(desc != nullptr);
    REQUIRE(desc->old_text == "Constellation of The Tower with a Prime Aspect of Secunda.");
    REQUIRE(desc->new_text == "");
    REQUIRE(desc->status == Tools::Status::untranslated);

    const auto * gmst = dict.at(Tools::RecType::GMST).find("s3dAudio");
    REQUIRE(gmst != nullptr);
    REQUIRE(gmst->old_text == "3D Audio");
    REQUIRE(gmst->new_text == "");
    REQUIRE(gmst->status == Tools::Status::untranslated);

    const auto * fnam = dict.at(Tools::RecType::FNAM).find("ACTI^A_Ex_De_Oar");
    REQUIRE(fnam != nullptr);
    REQUIRE(fnam->old_text == "Oar");
    REQUIRE(fnam->new_text == "");
    REQUIRE(fnam->status == Tools::Status::untranslated);

    const auto * info = dict.at(Tools::RecType::INFO).find("G^Greeting 0^1046442603030210731");
    REQUIRE(info != nullptr);
    REQUIRE(info->old_text == "I've heard you've got a price on your head, %PCRank. For a small fee, I can take care of that.");
    REQUIRE(info->new_text == "");
    REQUIRE(info->status == Tools::Status::untranslated);

    const auto * text = dict.at(Tools::RecType::TEXT).find("BookSkill_Acrobatics2");
    REQUIRE(text != nullptr);
    REQUIRE(text->old_text.size() == 10305);
    REQUIRE(text->new_text == "");
    REQUIRE(text->status == Tools::Status::untranslated);
}

TEST_CASE("make-base, same record order", "[i]")
{
    DictCreator esm(g_masterPath + "pl/Morrowind.esm", g_masterPath + "en/Morrowind.esm");
    const auto & dict = esm.getDict();

    const auto * cell = dict.at(Tools::RecType::CELL).find("Abaelun Mine");
    REQUIRE(cell != nullptr);
    REQUIRE(cell->old_text == "Abaelun Mine");
    REQUIRE(cell->new_text == "Kopalnia Abaelun");

    const auto * dial = dict.at(Tools::RecType::DIAL).find("Abebaal Egg Mine");
    REQUIRE(dial != nullptr);
    REQUIRE(dial->old_text == "Abebaal Egg Mine");
    REQUIRE(dial->new_text == "kopalnia jaj Abebaal");

    const auto * fnam = dict.at(Tools::RecType::FNAM).find("ACTI^A_Ex_De_Oar");
    REQUIRE(fnam != nullptr);
    REQUIRE(fnam->old_text == "Oar");

    const auto * indx = dict.at(Tools::RecType::INDX).find("MGEF^000");
    REQUIRE(indx != nullptr);
    REQUIRE(indx->old_text == "This effect permits the subject to breathe underwater for the duration.");

    const auto * gmst = dict.at(Tools::RecType::GMST).find("s3dAudio");
    REQUIRE(gmst != nullptr);
    REQUIRE(gmst->old_text == "3D Audio");

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
    REQUIRE(cell->old_text == "Abaelun Mine");
    REQUIRE(cell->new_text == "Abaelun-Mine");

    const auto * cell_missing = dict.at(Tools::RecType::CELL).find("Aharunartus");
    REQUIRE(cell_missing != nullptr);
    REQUIRE(cell_missing->old_text == "Aharunartus");
    REQUIRE(cell_missing->new_text == "MISSING");

    const auto * dial = dict.at(Tools::RecType::DIAL).find("Abebaal Egg Mine");
    REQUIRE(dial != nullptr);
    REQUIRE(dial->old_text == "Abebaal Egg Mine");
    REQUIRE(dial->new_text == "Eiermine Abebaal");

    const auto * fnam = dict.at(Tools::RecType::FNAM).find("ACTI^A_Ex_De_Oar");
    REQUIRE(fnam != nullptr);
    REQUIRE(fnam->old_text == "Oar");

    const auto * gmst = dict.at(Tools::RecType::GMST).find("s3dAudio");
    REQUIRE(gmst != nullptr);
    REQUIRE(gmst->old_text == "3D Audio");
}

TEST_CASE("make-base, DOUBLED entries preserve old_text", "[i]")
{
    DictCreator esm(g_masterPath + "pl/Morrowind.esm", g_masterPath + "en/Morrowind.esm");
    const auto & dict = esm.getDict();

    const auto & glossary = dict.at(Tools::RecType::Glossary);
    bool found_doubled = false;
    for (const auto & entry : glossary.records)
    {
        if (entry.key_text.find("^DOUBLED_") == std::string::npos)
            continue;

        found_doubled = true;
        auto base_id = entry.key_text.substr(0, entry.key_text.find("^DOUBLED_"));
        REQUIRE(entry.old_text == base_id);
        REQUIRE(entry.status == "");
        break;
    }
    REQUIRE(found_doubled);
}
