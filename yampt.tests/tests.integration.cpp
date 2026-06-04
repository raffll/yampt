#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esm_reader.hpp"
#include "../yampt/dict_creator.hpp"
#include "../yampt/dict_writer.hpp"
#include "../yampt/dict_reader.hpp"
#include "../yampt/dict_merger.hpp"

#include <filesystem>

extern std::string g_master_path;

namespace fs = std::filesystem;

static const std::string test_dir = "tests";

static const tools_t::record_entry_t * find_by_new_text(
    const tools_t::chapter_t & chapter, const std::string & text)
{
	for (const auto & entry : chapter.records)
	{
		if (entry.new_text == text)
			return &entry;
	}
	return nullptr;
}

TEST_CASE("cleanup test output directory", "[i]")
{
	fs::remove_all(test_dir);
	fs::create_directories(test_dir);
	REQUIRE(fs::is_directory(test_dir));
}

TEST_CASE("make EN, no base dict", "[i]")
{
	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "en/" + name + ".esm");
		const auto & dict = creator.get_dict();

		auto total = tools_t::get_number_of_elements_in_dict(dict);
		REQUIRE(total > 0);

		REQUIRE_FALSE(dict.at(tools_t::rec_type_t::cell).empty());
		REQUIRE_FALSE(dict.at(tools_t::rec_type_t::dial).empty());
		REQUIRE_FALSE(dict.at(tools_t::rec_type_t::info).empty());

		auto output = test_dir + "/" + name + "_en.json";
		dict_writer_t::write(dict, output);
		REQUIRE(fs::exists(output));
		REQUIRE(fs::file_size(output) > 0);
	}
}

TEST_CASE("make-base EN to PL", "[i]")
{
	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "pl/" + name + ".esm", g_master_path + "en/" + name + ".esm");
		const auto & dict = creator.get_dict();

		auto total = tools_t::get_number_of_elements_in_dict(dict);
		REQUIRE(total > 0);

		const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
		REQUIRE_FALSE(cell_chapter.empty());

		bool found_translated = false;
		for (const auto & entry : cell_chapter.records)
		{
			if (!entry.new_text.empty())
			{
				found_translated = true;
				break;
			}
		}
		REQUIRE(found_translated);

		if (std::string(name) == "Morrowind")
		{
			const auto * entry = find_by_new_text(cell_chapter, "Kopalnia Abaelun");
			REQUIRE(entry != nullptr);
			REQUIRE(entry->old_text == "Abaelun Mine");
		}

		auto output = test_dir + "/" + name + "_en_pl.json";
		dict_writer_t::write(dict, output);
		REQUIRE(fs::exists(output));
	}
}

TEST_CASE("make-base EN to DE", "[i]")
{
	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "de/" + name + ".esm", g_master_path + "en/" + name + ".esm");
		const auto & dict = creator.get_dict();

		auto total = tools_t::get_number_of_elements_in_dict(dict);
		REQUIRE(total > 0);

		const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
		REQUIRE_FALSE(cell_chapter.empty());

		bool found_translated = false;
		for (const auto & entry : cell_chapter.records)
		{
			if (!entry.new_text.empty())
			{
				found_translated = true;
				break;
			}
		}
		REQUIRE(found_translated);

		if (std::string(name) == "Morrowind")
		{
			const auto * entry = find_by_new_text(cell_chapter, "Abaelun-Mine");
			REQUIRE(entry != nullptr);
			REQUIRE(entry->old_text == "Abaelun Mine");
		}

		auto output = test_dir + "/" + name + "_en_de.json";
		dict_writer_t::write(dict, output);
		REQUIRE(fs::exists(output));
	}
}

TEST_CASE("merge EN to PL", "[i]")
{
	dict_merger_t merger(
	    {
	        test_dir + "/Morrowind_en_pl.json",
	        test_dir + "/Tribunal_en_pl.json",
	        test_dir + "/Bloodmoon_en_pl.json",
	    });
	const auto & dict = merger.get_dict();

	auto total = tools_t::get_number_of_elements_in_dict(dict);
	REQUIRE(total > 0);

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
	REQUIRE_FALSE(cell_chapter.empty());

	const auto * morrowind_entry = find_by_new_text(cell_chapter, "Kopalnia Abaelun");
	REQUIRE(morrowind_entry != nullptr);

	auto output = test_dir + "/Merged_en_pl.json";
	dict_writer_t::write(dict, output);
	REQUIRE(fs::exists(output));
}

TEST_CASE("make EN with base dict", "[i]")
{
	dict_reader_t reader(test_dir + "/Morrowind_en_pl.json");
	REQUIRE(reader.is_loaded());

	dict_creator_t creator(g_master_path + "en/Morrowind.esm", &reader.get_dict());
	const auto & dict = creator.get_dict();

	auto total = tools_t::get_number_of_elements_in_dict(dict);
	REQUIRE(total > 0);

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
	const auto * entry = find_by_new_text(cell_chapter, "Kopalnia Abaelun");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->old_text == "Abaelun Mine");
	REQUIRE(entry->status != tools_t::status_t::untranslated);

	auto output = test_dir + "/Morrowind_en_with_base.json";
	dict_writer_t::write(dict, output);
	REQUIRE(fs::exists(output));
}
