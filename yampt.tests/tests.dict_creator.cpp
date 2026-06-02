#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esm_reader.hpp"
#include "../yampt/dict_creator.hpp"

#include <string>

extern std::string g_master_path;

TEST_CASE("make without base dict produces untranslated status", "[i]")
{
	dict_creator_t esm(g_master_path + "en/Morrowind.esm");
	const auto & dict = esm.get_dict();

	const auto * cell = dict.at(tools_t::rec_type_t::cell).find("Abaelun Mine");
	REQUIRE(cell != nullptr);
	REQUIRE(cell->old_text == "Abaelun Mine");
	REQUIRE(cell->new_text == "");
	REQUIRE(cell->status == tools_t::status_t::untranslated);

	const auto * dial = dict.at(tools_t::rec_type_t::dial).find("1000-drake pledge");
	REQUIRE(dial != nullptr);
	REQUIRE(dial->old_text == "1000-drake pledge");
	REQUIRE(dial->new_text == "");
	REQUIRE(dial->status == tools_t::status_t::untranslated);

	const auto * indx = dict.at(tools_t::rec_type_t::indx).find("MGEF^000");
	REQUIRE(indx != nullptr);
	REQUIRE(indx->old_text == "This effect permits the subject to breathe underwater for the duration.");
	REQUIRE(indx->new_text == "");
	REQUIRE(indx->status == tools_t::status_t::untranslated);

	const auto * rnam = dict.at(tools_t::rec_type_t::rnam).find("Ashlanders^0");
	REQUIRE(rnam != nullptr);
	REQUIRE(rnam->old_text == "Clanfriend");
	REQUIRE(rnam->new_text == "");
	REQUIRE(rnam->status == tools_t::status_t::untranslated);

	const auto * desc = dict.at(tools_t::rec_type_t::desc).find("BSGN^Beggar's Nose");
	REQUIRE(desc != nullptr);
	REQUIRE(desc->old_text == "Constellation of The Tower with a Prime Aspect of Secunda.");
	REQUIRE(desc->new_text == "");
	REQUIRE(desc->status == tools_t::status_t::untranslated);

	const auto * gmst = dict.at(tools_t::rec_type_t::gmst).find("s3dAudio");
	REQUIRE(gmst != nullptr);
	REQUIRE(gmst->old_text == "3D Audio");
	REQUIRE(gmst->new_text == "");
	REQUIRE(gmst->status == tools_t::status_t::untranslated);

	const auto * fnam = dict.at(tools_t::rec_type_t::fnam).find("ACTI^A_Ex_De_Oar");
	REQUIRE(fnam != nullptr);
	REQUIRE(fnam->old_text == "Oar");
	REQUIRE(fnam->new_text == "");
	REQUIRE(fnam->status == tools_t::status_t::untranslated);

	const auto * info = dict.at(tools_t::rec_type_t::info).find("G^Greeting 0^1046442603030210731");
	REQUIRE(info != nullptr);
	REQUIRE(
	    info->old_text ==
	    "I've heard you've got a price on your head, %PCRank. For a small fee, I can take care of that.");
	REQUIRE(info->new_text == "");
	REQUIRE(info->status == tools_t::status_t::untranslated);

	const auto * text = dict.at(tools_t::rec_type_t::text).find("BookSkill_Acrobatics2");
	REQUIRE(text != nullptr);
	REQUIRE(text->old_text.size() == 10305);
	REQUIRE(text->new_text == "");
	REQUIRE(text->status == tools_t::status_t::untranslated);
}

TEST_CASE("make-base, same record order", "[i]")
{
	dict_creator_t esm(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = esm.get_dict();

	const auto * cell = dict.at(tools_t::rec_type_t::cell).find("Abaelun Mine");
	REQUIRE(cell != nullptr);
	REQUIRE(cell->old_text == "Abaelun Mine");
	REQUIRE(cell->new_text == "Kopalnia Abaelun");

	const auto * dial = dict.at(tools_t::rec_type_t::dial).find("Abebaal Egg Mine");
	REQUIRE(dial != nullptr);
	REQUIRE(dial->old_text == "Abebaal Egg Mine");
	REQUIRE(dial->new_text == "kopalnia jaj Abebaal");

	const auto * fnam = dict.at(tools_t::rec_type_t::fnam).find("ACTI^A_Ex_De_Oar");
	REQUIRE(fnam != nullptr);
	REQUIRE(fnam->old_text == "Oar");

	const auto * indx = dict.at(tools_t::rec_type_t::indx).find("MGEF^000");
	REQUIRE(indx != nullptr);
	REQUIRE(indx->old_text == "This effect permits the subject to breathe underwater for the duration.");

	const auto * gmst = dict.at(tools_t::rec_type_t::gmst).find("s3dAudio");
	REQUIRE(gmst != nullptr);
	REQUIRE(gmst->old_text == "3D Audio");

	const auto * info = dict.at(tools_t::rec_type_t::info).find("T^kopalnia jaj Abebaal^3253555431180022526");
	REQUIRE(info != nullptr);

	const auto * info_en = dict.at(tools_t::rec_type_t::info).find("T^Abebaal Egg Mine^3253555431180022526");
	REQUIRE(info_en == nullptr);
}

TEST_CASE("make-base, different record order", "[i]")
{
	dict_creator_t esm(g_master_path + "de/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = esm.get_dict();

	const auto * cell = dict.at(tools_t::rec_type_t::cell).find("Abaelun Mine");
	REQUIRE(cell != nullptr);
	REQUIRE(cell->old_text == "Abaelun Mine");
	REQUIRE(cell->new_text == "Abaelun-Mine");

	const auto * cell_missing = dict.at(tools_t::rec_type_t::cell).find("Aharunartus");
	REQUIRE(cell_missing != nullptr);
	REQUIRE(cell_missing->old_text == "Aharunartus");
	REQUIRE(cell_missing->new_text == "MISSING");

	const auto * dial = dict.at(tools_t::rec_type_t::dial).find("Abebaal Egg Mine");
	REQUIRE(dial != nullptr);
	REQUIRE(dial->old_text == "Abebaal Egg Mine");
	REQUIRE(dial->new_text == "Eiermine Abebaal");

	const auto * fnam = dict.at(tools_t::rec_type_t::fnam).find("ACTI^A_Ex_De_Oar");
	REQUIRE(fnam != nullptr);
	REQUIRE(fnam->old_text == "Oar");

	const auto * gmst = dict.at(tools_t::rec_type_t::gmst).find("s3dAudio");
	REQUIRE(gmst != nullptr);
	REQUIRE(gmst->old_text == "3D Audio");
}

TEST_CASE("make-base, DOUBLED entries preserve old_text", "[i]")
{
	dict_creator_t esm(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = esm.get_dict();

	const auto & glossary = dict.at(tools_t::rec_type_t::glossary);
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
