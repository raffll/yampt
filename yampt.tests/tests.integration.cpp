#include <catch2/catch_all.hpp>
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

static void flush_log(const std::string & lang, const std::string & name)
{
	auto log = tools_t::get_log();
	if (log.empty())
		return;

	auto path = test_dir + "/" + lang + "/" + name + ".log";
	tools_t::write_text(log, path);
	tools_t::reset_log();
}

TEST_CASE("cleanup test output directory", "[i]")
{
	fs::remove_all(test_dir);
	fs::create_directories(test_dir + "/en");
	fs::create_directories(test_dir + "/pl");
	fs::create_directories(test_dir + "/de");
	fs::create_directories(test_dir + "/fr");
	tools_t::reset_log();
}

TEST_CASE("make EN, no base dict", "[i]")
{
	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "en/" + name + ".esm");
		dict_writer_t::write(creator.get_dict(), test_dir + "/en/" + name + "_en.json");
		flush_log("en", std::string(name) + "_en");
	}
}

TEST_CASE("make-base EN to PL", "[i]")
{
	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "pl/" + name + ".esm", g_master_path + "en/" + name + ".esm");
		dict_writer_t::write(creator.get_dict(), test_dir + "/pl/" + name + "_en_pl.json");
		flush_log("pl", std::string(name) + "_en_pl");
	}
}

TEST_CASE("make-base EN to DE", "[i]")
{
	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "de/" + name + ".esm", g_master_path + "en/" + name + ".esm");
		dict_writer_t::write(creator.get_dict(), test_dir + "/de/" + name + "_en_de.json");
		flush_log("de", std::string(name) + "_en_de");
	}
}

TEST_CASE("make-base EN to FR", "[i]")
{
	for (const auto & name : { "Morrowind", "Tribunal", "Bloodmoon" })
	{
		dict_creator_t creator(g_master_path + "fr/" + name + ".esm", g_master_path + "en/" + name + ".esm");
		dict_writer_t::write(creator.get_dict(), test_dir + "/fr/" + name + "_en_fr.json");
		flush_log("fr", std::string(name) + "_en_fr");
	}
}

TEST_CASE("merge EN to PL", "[i]")
{
	dict_merger_t merger(
	    {
	        test_dir + "/pl/Morrowind_en_pl.json",
	        test_dir + "/pl/Tribunal_en_pl.json",
	        test_dir + "/pl/Bloodmoon_en_pl.json",
	    });
	dict_writer_t::write(merger.get_dict(), test_dir + "/pl/Merged_en_pl.json");
	flush_log("pl", "Merged_en_pl");
}

TEST_CASE("make EN with base dict", "[i]")
{
	dict_reader_t reader(test_dir + "/pl/Morrowind_en_pl.json");
	dict_creator_t creator(g_master_path + "en/Morrowind.esm", &reader.get_dict());
	dict_writer_t::write(creator.get_dict(), test_dir + "/en/Morrowind_en_with_base.json");
	flush_log("en", "Morrowind_en_with_base");
}
