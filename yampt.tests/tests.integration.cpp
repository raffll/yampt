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
static const std::string json_dir = test_dir + "/json";
static const std::string logs_dir = test_dir + "/logs";

static void flush_log(const std::string & name)
{
	auto log = tools_t::get_log();
	if (log.empty())
		return;

	auto path = logs_dir + "/" + name + ".log";
	tools_t::write_text(log, path);
	tools_t::reset_log();
}

static void flush_log(const std::string & lang, const std::string & name)
{
	auto log = tools_t::get_log();
	if (log.empty())
		return;

	auto path = logs_dir + "/" + lang + "/" + name + ".log";
	tools_t::write_text(log, path);
	tools_t::reset_log();
}

static const tools_t::record_entry_t * find_by_new_text(const tools_t::chapter_t & chapter, const std::string & text)
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
	fs::create_directories(json_dir);
	fs::create_directories(logs_dir + "/en");
	fs::create_directories(logs_dir + "/pl");
	fs::create_directories(logs_dir + "/de");
	fs::create_directories(logs_dir + "/fr");
	REQUIRE(fs::is_directory(test_dir));
	tools_t::reset_log();
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

		auto output = json_dir + "/" + name + "_en.json";
		dict_writer_t::write(dict, output);
		REQUIRE(fs::exists(output));
		REQUIRE(fs::file_size(output) > 0);

		flush_log("en", std::string(name) + "_en");
	}
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

	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "pl/" + name + ".esm", g_master_path + "en/" + name + ".esm", &engine);
		const auto & dict = creator.get_dict();

		auto total = tools_t::get_number_of_elements_in_dict(dict);
		REQUIRE(total > 0);

		const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
		REQUIRE_FALSE(cell_chapter.empty());

		if (std::string(name) == "Morrowind")
		{
			const auto * entry = find_by_new_text(cell_chapter, "Kopalnia Abaelun");
			REQUIRE(entry != nullptr);
			REQUIRE(entry->old_text == "Abaelun Mine");
		}

		auto output = json_dir + "/" + name + "_en_pl.json";
		dict_writer_t::write(dict, output);
		REQUIRE(fs::exists(output));

		flush_log("pl", std::string(name) + "_en_pl");
	}
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

	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "de/" + name + ".esm", g_master_path + "en/" + name + ".esm", &engine);
		const auto & dict = creator.get_dict();

		auto total = tools_t::get_number_of_elements_in_dict(dict);
		REQUIRE(total > 0);

		const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
		REQUIRE_FALSE(cell_chapter.empty());

		if (std::string(name) == "Morrowind")
		{
			const auto * entry = find_by_new_text(cell_chapter, "Abaelun-Mine");
			REQUIRE(entry != nullptr);
			REQUIRE(entry->old_text == "Abaelun Mine");
		}

		auto output = json_dir + "/" + name + "_en_de.json";
		dict_writer_t::write(dict, output);
		REQUIRE(fs::exists(output));

		flush_log("de", std::string(name) + "_en_de");
	}
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

	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "fr/" + name + ".esm", g_master_path + "en/" + name + ".esm", &engine);
		const auto & dict = creator.get_dict();

		auto total = tools_t::get_number_of_elements_in_dict(dict);
		REQUIRE(total > 0);

		const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
		REQUIRE_FALSE(cell_chapter.empty());

		auto output = json_dir + "/" + name + "_en_fr.json";
		dict_writer_t::write(dict, output);
		REQUIRE(fs::exists(output));

		flush_log("fr", std::string(name) + "_en_fr");
	}
}

TEST_CASE("merge EN to PL", "[i]")
{
	dict_merger_t merger(
	    {
	        json_dir + "/Morrowind_en_pl.json",
	        json_dir + "/Tribunal_en_pl.json",
	        json_dir + "/Bloodmoon_en_pl.json",
	    });
	const auto & dict = merger.get_dict();

	auto total = tools_t::get_number_of_elements_in_dict(dict);
	REQUIRE(total > 0);

	const auto & cell_chapter = dict.at(tools_t::rec_type_t::cell);
	REQUIRE_FALSE(cell_chapter.empty());

	const auto * morrowind_entry = find_by_new_text(cell_chapter, "Kopalnia Abaelun");
	REQUIRE(morrowind_entry != nullptr);

	auto output = json_dir + "/Merged_en_pl.json";
	dict_writer_t::write(dict, output);
	REQUIRE(fs::exists(output));
	flush_log("pl", "Merged_en_pl");
}

TEST_CASE("make EN with base dict", "[i]")
{
	dict_reader_t reader(json_dir + "/Morrowind_en_pl.json");
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

	auto output = json_dir + "/Morrowind_en_with_base.json";
	dict_writer_t::write(dict, output);
	REQUIRE(fs::exists(output));
	flush_log("en", "Morrowind_en_with_base");
}
