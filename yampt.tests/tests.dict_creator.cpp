#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dict_creator.hpp"
#include "../yampt/dict_writer.hpp"
#include "../yampt/dict_reader.hpp"

#include <filesystem>

extern std::string g_master_path;

namespace fs = std::filesystem;

TEST_CASE("dict_creator make mode without base dict sets untranslated", "[i]")
{
	dict_creator_t creator(g_master_path + "en/Morrowind.esm");
	const auto & dict = creator.get_dict();

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
	REQUIRE_FALSE(cell_chapter.empty());

	for (const auto & entry : cell_chapter.records)
	{
		REQUIRE(entry.status == tools_t::status_t::untranslated);
		REQUIRE(entry.new_text == entry.old_text);
	}
}

TEST_CASE("dict_creator make-base mode sets matched_by_coords on cells", "[i]")
{
	dict_creator_t creator(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = creator.get_dict();

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
	REQUIRE_FALSE(cell_chapter.empty());

	bool found_matched = false;
	for (const auto & entry : cell_chapter.records)
	{
		if (entry.status == tools_t::status_t::matched_by_coords && entry.new_text != entry.old_text)
		{
			found_matched = true;
			break;
		}
	}
	REQUIRE(found_matched);
}

TEST_CASE("dict_creator make-base mode sets missing for unmatched cells", "[i]")
{
	dict_creator_t creator(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = creator.get_dict();

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);

	for (const auto & entry : cell_chapter.records)
	{
		if (entry.status == tools_t::status_t::missing)
		{
			REQUIRE(entry.new_text == entry.old_text);
		}
	}
}

TEST_CASE("dict_creator make-base mode sets wilderness status", "[i]")
{
	dict_creator_t creator(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = creator.get_dict();

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);

	bool found_wilderness = false;
	for (const auto & entry : cell_chapter.records)
	{
		if (entry.status == tools_t::status_t::wilderness)
		{
			found_wilderness = true;
			break;
		}
	}
	REQUIRE(found_wilderness);
}

TEST_CASE("dict_creator make-base mode sets region status", "[i]")
{
	dict_creator_t creator(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = creator.get_dict();

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);

	bool found_region = false;
	for (const auto & entry : cell_chapter.records)
	{
		if (entry.status == tools_t::status_t::region)
		{
			found_region = true;
			break;
		}
	}
	REQUIRE(found_region);
}

TEST_CASE("dict_creator make-base mode duplicate uses ^DUP_ suffix", "[i]")
{
	dict_creator_t creator(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = creator.get_dict();

	bool found_dup = false;
	for (const auto & [type, chapter] : dict)
	{
		for (const auto & entry : chapter.records)
		{
			if (entry.key_text.find("^DUP_") != std::string::npos)
			{
				found_dup = true;
				REQUIRE(entry.status == tools_t::status_t::duplicate);
				break;
			}
		}
		if (found_dup)
			break;
	}
}

TEST_CASE("dict_creator make-base mode does not use ^DOUBLED_ suffix", "[i]")
{
	dict_creator_t creator(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = creator.get_dict();

	for (const auto & [type, chapter] : dict)
	{
		for (const auto & entry : chapter.records)
		{
			REQUIRE(entry.key_text.find("^DOUBLED_") == std::string::npos);
		}
	}
}

TEST_CASE("dict_creator make-base mode populates speaker fields on info records", "[i]")
{
	dict_creator_t creator(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = creator.get_dict();

	const auto & info_chapter = dict.at(tools_t::rec_type_t::info);
	REQUIRE_FALSE(info_chapter.empty());

	bool found_speaker = false;
	for (const auto & entry : info_chapter.records)
	{
		if (!entry.speaker.empty())
		{
			found_speaker = true;
			REQUIRE_FALSE(entry.speaker_name.empty());
			REQUIRE((entry.gender == "M" || entry.gender == "F"));
			break;
		}
	}
	REQUIRE(found_speaker);
}

TEST_CASE("dict_creator make mode with base dict assigns auto_base", "[i]")
{
	dict_creator_t base_creator(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & base_dict = base_creator.get_dict();

	dict_creator_t creator(g_master_path + "en/Morrowind.esm", &base_dict);
	const auto & dict = creator.get_dict();

	bool found_auto_base = false;
	for (const auto & [type, chapter] : dict)
	{
		for (const auto & entry : chapter.records)
		{
			if (entry.status == tools_t::status_t::auto_base)
			{
				found_auto_base = true;
				REQUIRE(entry.new_text != entry.old_text);
				break;
			}
		}
		if (found_auto_base)
			break;
	}
	REQUIRE(found_auto_base);
}

TEST_CASE("dict_creator make mode with base dict assigns auto_identical", "[i]")
{
	dict_creator_t base_creator(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & base_dict = base_creator.get_dict();

	dict_creator_t creator(g_master_path + "en/Morrowind.esm", &base_dict);
	const auto & dict = creator.get_dict();

	bool found_auto_identical = false;
	for (const auto & [type, chapter] : dict)
	{
		for (const auto & entry : chapter.records)
		{
			if (entry.status == tools_t::status_t::auto_identical)
			{
				found_auto_identical = true;
				REQUIRE(entry.new_text == entry.old_text);
				break;
			}
		}
		if (found_auto_identical)
			break;
	}
	REQUIRE(found_auto_identical);
}

TEST_CASE("dict_creator make-base mode does not produce npc_flag chapter", "[i]")
{
	dict_creator_t creator(g_master_path + "pl/Morrowind.esm", g_master_path + "en/Morrowind.esm");
	const auto & dict = creator.get_dict();

	for (const auto & [type, chapter] : dict)
	{
		std::string type_str = tools_t::type_to_str(type);
		REQUIRE(type_str != "NPC_FLAG");
	}
}

TEST_CASE("dict_creator make mode new_text is never empty", "[i]")
{
	dict_creator_t creator(g_master_path + "en/Morrowind.esm");
	const auto & dict = creator.get_dict();

	for (const auto & [type, chapter] : dict)
	{
		for (const auto & entry : chapter.records)
		{
			INFO("type=" << tools_t::type_to_str(type) << " key=" << entry.key_text);
			REQUIRE_FALSE(entry.new_text.empty());
		}
	}
}
