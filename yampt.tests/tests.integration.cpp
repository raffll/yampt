#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esm_reader.hpp"
#include "../yampt/dict_creator.hpp"
#include "../yampt/dict_writer.hpp"
#include "../yampt/dict_reader.hpp"
#include "../yampt/dict_merger.hpp"
#include "../yampt/translation_engine.hpp"

#include <filesystem>

extern std::string g_master_path;

namespace fs = std::filesystem;

static const std::string test_dir = "tests";

static int log_counter = 0;

static void flush_log(const std::string & test_name)
{
	auto log = tools_t::get_log();
	if (log.empty())
		return;

	std::string safe_name = test_name;
	for (auto & c : safe_name)
	{
		if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
			c = '_';
	}

	auto path = test_dir + "/" + std::to_string(++log_counter) + "_" + safe_name + ".log";
	tools_t::write_text(log, path);
	tools_t::reset_log();
}

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
	flush_log("cleanup");
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
	flush_log("make_EN_no_base_dict");
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
	flush_log("make-base_EN_to_PL");
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
	flush_log("make-base_EN_to_DE");
}

TEST_CASE("make-base EN to FR", "[i]")
{
	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "fr/" + name + ".esm", g_master_path + "en/" + name + ".esm");
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

		auto output = test_dir + "/" + name + "_en_fr.json";
		dict_writer_t::write(dict, output);
		REQUIRE(fs::exists(output));
	}
	flush_log("make-base_EN_to_FR");
}

TEST_CASE("log missing and duplicate cells to file", "[i]")
{
	for (const auto & lang : { "pl", "de", "fr" })
	{
		for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
		{
			dict_creator_t creator(
			    g_master_path + lang + "/" + name + ".esm",
			    g_master_path + "en/" + name + ".esm");
			const auto & dict = creator.get_dict();
			const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);

			for (const auto & entry : cell_chapter.records)
			{
				if (entry.status == tools_t::status_t::missing)
					tools_t::add_log("[missing] " + std::string(lang) + "/" + name + ": " + entry.old_text + "\r\n", true);
				if (entry.status == tools_t::status_t::duplicate)
					tools_t::add_log("[duplicate] " + std::string(lang) + "/" + name + ": " + entry.key_text + " | old=" + entry.old_text + " | new=" + entry.new_text + "\r\n", true);
			}
		}
	}
	flush_log("missing_and_duplicate_cells");
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
	flush_log("merge_EN_to_PL");
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
	flush_log("make_EN_with_base_dict");
}

TEST_CASE("make-base EN to DE with translation engine", "[i]")
{
	translation_engine_t engine;
	bool loaded = engine.load("../../models/en-de");
	if (!loaded)
	{
		WARN("Skipping: models/en-de not found");
		return;
	}

	dict_creator_t creator(
	    g_master_path + "de/Morrowind.esm",
	    g_master_path + "en/Morrowind.esm",
	    &engine);
	const auto & dict = creator.get_dict();

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
	REQUIRE_FALSE(cell_chapter.empty());

	int heuristic_matches = 0;
	for (const auto & entry : cell_chapter.records)
	{
		if (entry.status == tools_t::status_t::matched_by_heuristic ||
		    entry.status == tools_t::status_t::matched_by_name)
			heuristic_matches++;
	}

	CAPTURE(heuristic_matches);
	REQUIRE(heuristic_matches > 0);

	auto output = test_dir + "/Morrowind_en_de_translated.json";
	dict_writer_t::write(dict, output);
	REQUIRE(fs::exists(output));
	flush_log("make-base_EN_to_DE_translated");
}

TEST_CASE("make-base EN to PL with translation engine", "[i]")
{
	translation_engine_t engine;
	bool loaded = engine.load("../../models/en-pl");
	if (!loaded)
	{
		WARN("Skipping: models/en-pl not found");
		return;
	}

	dict_creator_t creator(
	    g_master_path + "pl/Morrowind.esm",
	    g_master_path + "en/Morrowind.esm",
	    &engine);
	const auto & dict = creator.get_dict();

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
	REQUIRE_FALSE(cell_chapter.empty());

	int heuristic_matches = 0;
	for (const auto & entry : cell_chapter.records)
	{
		if (entry.status == tools_t::status_t::matched_by_heuristic ||
		    entry.status == tools_t::status_t::matched_by_name)
			heuristic_matches++;
	}

	CAPTURE(heuristic_matches);
	REQUIRE(heuristic_matches > 0);

	auto output = test_dir + "/Morrowind_en_pl_translated.json";
	dict_writer_t::write(dict, output);
	REQUIRE(fs::exists(output));
	flush_log("make-base_EN_to_PL_translated");
}

TEST_CASE("make-base EN to FR with translation engine", "[i]")
{
	translation_engine_t engine;
	bool loaded = engine.load("../../models/en-fr");
	if (!loaded)
	{
		WARN("Skipping: models/en-fr not found");
		return;
	}

	dict_creator_t creator(
	    g_master_path + "fr/Morrowind.esm",
	    g_master_path + "en/Morrowind.esm",
	    &engine);
	const auto & dict = creator.get_dict();

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
	REQUIRE_FALSE(cell_chapter.empty());

	int heuristic_matches = 0;
	for (const auto & entry : cell_chapter.records)
	{
		if (entry.status == tools_t::status_t::matched_by_heuristic ||
		    entry.status == tools_t::status_t::matched_by_name)
			heuristic_matches++;
	}

	CAPTURE(heuristic_matches);
	REQUIRE(heuristic_matches > 0);

	auto output = test_dir + "/Morrowind_en_fr_translated.json";
	dict_writer_t::write(dict, output);
	REQUIRE(fs::exists(output));
	flush_log("make-base_EN_to_FR_translated");
}

TEST_CASE("log missing cells with translation engine", "[i]")
{
	struct model_info
	{
		const char * lang;
		const char * model_path;
	};

	model_info models[] = {
	    { "de", "../../models/en-de" },
	    { "pl", "../../models/en-pl" },
	    { "fr", "../../models/en-fr" },
	};

	for (const auto & m : models)
	{
		translation_engine_t engine;
		if (!engine.load(m.model_path))
			continue;

		for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
		{
			dict_creator_t creator(
			    g_master_path + m.lang + "/" + name + ".esm",
			    g_master_path + "en/" + name + ".esm",
			    &engine);
			const auto & dict = creator.get_dict();
			const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);

			for (const auto & entry : cell_chapter.records)
			{
				if (entry.status == tools_t::status_t::missing)
					tools_t::add_log("[missing] " + std::string(m.lang) + "/" + name + ": " + entry.old_text + "\r\n", true);
			}
		}
	}
	flush_log("missing_cells_translated");
}
