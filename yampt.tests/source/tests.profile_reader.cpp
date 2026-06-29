#include <catch2/catch_all.hpp>
#include <io/profile_reader.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>
#include <fstream>

namespace {

std::string temp_file(const std::string & filename)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / filename).string();
	return string_utils::normalize_path(path);
}

void write_text_file(const std::string & path, const std::string & content)
{
	std::ofstream output(path);
	output << content;
}

void cleanup_file(const std::string & path)
{
	std::error_code error_code;
	std::filesystem::remove(path, error_code);
}

} // anonymous namespace

TEST_CASE("parse_loadorder_file, returns plugin names", "[i]")
{
	const auto path = temp_file("yampt_test_loadorder.txt");
	write_text_file(path, "Morrowind.esm\nTribunal.esm\nBloodmoon.esm\n");

	const auto result = parse_loadorder_file(path);

	REQUIRE(result.size() == 3);
	REQUIRE(result[0] == "Morrowind.esm");
	REQUIRE(result[1] == "Tribunal.esm");
	REQUIRE(result[2] == "Bloodmoon.esm");

	cleanup_file(path);
}

TEST_CASE("parse_loadorder_file, skips comments and empty lines", "[i]")
{
	const auto path = temp_file("yampt_test_loadorder2.txt");
	write_text_file(path, "# comment line\n\nMorrowind.esm\n\n# another\nTribunal.esm\n");

	const auto result = parse_loadorder_file(path);

	REQUIRE(result.size() == 2);
	REQUIRE(result[0] == "Morrowind.esm");
	REQUIRE(result[1] == "Tribunal.esm");

	cleanup_file(path);
}

TEST_CASE("parse_loadorder_file, strips star prefix", "[i]")
{
	const auto path = temp_file("yampt_test_loadorder3.txt");
	write_text_file(path, "*Morrowind.esm\n*plugin.esp\n");

	const auto result = parse_loadorder_file(path);

	REQUIRE(result.size() == 2);
	REQUIRE(result[0] == "Morrowind.esm");
	REQUIRE(result[1] == "plugin.esp");

	cleanup_file(path);
}

TEST_CASE("parse_modlist_file, returns enabled mods reversed", "[i]")
{
	const auto path = temp_file("yampt_test_modlist.txt");
	write_text_file(path, "+ModA\n+ModB\n+ModC\n");

	const auto result = parse_modlist_file(path);

	REQUIRE(result.size() == 3);
	REQUIRE(result[0] == "ModC");
	REQUIRE(result[1] == "ModB");
	REQUIRE(result[2] == "ModA");

	cleanup_file(path);
}

TEST_CASE("parse_modlist_file, skips disabled mods", "[i]")
{
	const auto path = temp_file("yampt_test_modlist2.txt");
	write_text_file(path, "+EnabledMod\n-DisabledMod\n+AnotherEnabled\n");

	const auto result = parse_modlist_file(path);

	REQUIRE(result.size() == 2);
	REQUIRE(result[0] == "AnotherEnabled");
	REQUIRE(result[1] == "EnabledMod");

	cleanup_file(path);
}

TEST_CASE("resolve_plugin_in_mods, finds file in mod folder", "[i]")
{
	namespace fs = std::filesystem;
	const auto mods_root = fs::temp_directory_path() / "yampt_test_mods";
	const auto mod_dir = mods_root / "TestMod";
	fs::create_directories(mod_dir);

	const auto plugin_path = mod_dir / "plugin.esp";
	std::ofstream(plugin_path.string()).put('x');

	const std::vector<std::string> enabled = { "TestMod" };
	const auto result = resolve_plugin_in_mods("plugin.esp", mods_root.string(), enabled, "");

	REQUIRE_FALSE(result.empty());
	REQUIRE(fs::path(result).filename() == "plugin.esp");

	fs::remove_all(mods_root);
}

TEST_CASE("resolve_plugin_in_mods, falls back to game data path", "[i]")
{
	namespace fs = std::filesystem;
	const auto game_data = fs::temp_directory_path() / "yampt_test_gamedata";
	fs::create_directories(game_data);

	const auto plugin_path = game_data / "fallback.esm";
	std::ofstream(plugin_path.string()).put('x');

	const std::vector<std::string> enabled = { "EmptyMod" };
	const auto mods_root = fs::temp_directory_path() / "yampt_test_mods_empty";
	fs::create_directories(mods_root / "EmptyMod");

	const auto result = resolve_plugin_in_mods("fallback.esm", mods_root.string(), enabled, game_data.string());

	REQUIRE_FALSE(result.empty());
	REQUIRE(fs::path(result).filename() == "fallback.esm");

	fs::remove_all(game_data);
	fs::remove_all(mods_root);
}

TEST_CASE("resolve_plugin_in_mods, returns empty when not found", "[i]")
{
	const std::vector<std::string> enabled = { "NonExistent" };
	const auto result = resolve_plugin_in_mods("missing.esp", "/fake/mods", enabled, "/fake/game");

	REQUIRE(result.empty());
}

TEST_CASE("parse_openmw_cfg_file, extracts data and content", "[i]")
{
	const auto path = temp_file("yampt_test_openmw.cfg");
	write_text_file(
	    path,
	    "data=\"/path/to/data1\"\n"
	    "data=\"/path/to/data2\"\n"
	    "content=Morrowind.esm\n"
	    "content=Tribunal.esm\n"
	    "fallback=key,value\n");

	const auto result = parse_openmw_cfg_file(path);

	REQUIRE(result.data_dirs.size() == 2);
	REQUIRE(result.data_dirs[0] == "/path/to/data1");
	REQUIRE(result.data_dirs[1] == "/path/to/data2");
	REQUIRE(result.content_names.size() == 2);
	REQUIRE(result.content_names[0] == "Morrowind.esm");
	REQUIRE(result.content_names[1] == "Tribunal.esm");

	cleanup_file(path);
}
