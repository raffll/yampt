#include <catch2/catch_all.hpp>
#include <io/profile_reader.hpp>
#include <set>
#include <string>

TEST_CASE("profile_reader_t::parse_openmw_cfg_text, extracts data and content", "[u]")
{
	const auto result = profile_reader_t::parse_openmw_cfg_text(
	    "data=\"/usr/share/morrowind/Data Files\"\n"
	    "data=\"/home/user/.config/openmw/data\"\n"
	    "content=Morrowind.esm\n"
	    "content=Tribunal.esm\n"
	    "content=MyMod.esp\n");

	REQUIRE(result.data_dirs.size() == 2);
	REQUIRE(result.data_dirs[0] == "/usr/share/morrowind/Data Files");
	REQUIRE(result.data_dirs[1] == "/home/user/.config/openmw/data");
	REQUIRE(result.content_names.size() == 3);
	REQUIRE(result.content_names[0] == "Morrowind.esm");
	REQUIRE(result.content_names[1] == "Tribunal.esm");
	REQUIRE(result.content_names[2] == "MyMod.esp");
}

TEST_CASE("profile_reader_t::parse_openmw_cfg_text, handles spaces around equals", "[u]")
{
	const auto result = profile_reader_t::parse_openmw_cfg_text(
	    "data =\"/path/one\"\n"
	    "content =Morrowind.esm\n");

	REQUIRE(result.data_dirs.size() == 1);
	REQUIRE(result.data_dirs[0] == "/path/one");
	REQUIRE(result.content_names.size() == 1);
	REQUIRE(result.content_names[0] == "Morrowind.esm");
}

TEST_CASE("profile_reader_t::parse_openmw_cfg_text, ignores other lines", "[u]")
{
	const auto result = profile_reader_t::parse_openmw_cfg_text(
	    "fallback=key,value\n"
	    "encoding=win1252\n"
	    "data=\"/only/this\"\n"
	    "no-sound=1\n");

	REQUIRE(result.data_dirs.size() == 1);
	REQUIRE(result.content_names.empty());
}

TEST_CASE("profile_reader_t::parse_openmw_cfg_text, data without quotes", "[u]")
{
	const auto result = profile_reader_t::parse_openmw_cfg_text(
	    "data=/simple/path\n");

	REQUIRE(result.data_dirs.size() == 1);
	REQUIRE(result.data_dirs[0] == "/simple/path");
}

TEST_CASE("profile_reader_t::parse_openmw_cfg_text, windows line endings", "[u]")
{
	const auto result = profile_reader_t::parse_openmw_cfg_text(
	    "data=\"C:/Games/Morrowind/Data Files\"\r\n"
	    "content=Morrowind.esm\r\n");

	REQUIRE(result.data_dirs.size() == 1);
	REQUIRE(result.data_dirs[0] == "C:/Games/Morrowind/Data Files");
	REQUIRE(result.content_names.size() == 1);
}

TEST_CASE("profile_reader_t::parse_mo2_profile_text, basic loadorder and modlist", "[u]")
{
	const auto result = profile_reader_t::parse_mo2_profile_text(
	    "Morrowind.esm\nTribunal.esm\nBloodmoon.esm\nMyMod.esp\n",
	    "+ModC\n+ModB\n+ModA\n");

	REQUIRE(result.plugin_names.size() == 4);
	REQUIRE(result.plugin_names[0] == "Morrowind.esm");
	REQUIRE(result.plugin_names[3] == "MyMod.esp");
	REQUIRE(result.enabled_mods.size() == 3);
	REQUIRE(result.enabled_mods[0] == "ModA");
	REQUIRE(result.enabled_mods[1] == "ModB");
	REQUIRE(result.enabled_mods[2] == "ModC");
}

TEST_CASE("profile_reader_t::parse_mo2_profile_text, strips star and skips comments", "[u]")
{
	const auto result = profile_reader_t::parse_mo2_profile_text(
	    "# This is a comment\n"
	    "*Morrowind.esm\n"
	    "\n"
	    "*Tribunal.esm\n",
	    "+OnlyMod\n");

	REQUIRE(result.plugin_names.size() == 2);
	REQUIRE(result.plugin_names[0] == "Morrowind.esm");
	REQUIRE(result.plugin_names[1] == "Tribunal.esm");
}

TEST_CASE("profile_reader_t::parse_mo2_profile_text, disabled mods excluded", "[u]")
{
	const auto result = profile_reader_t::parse_mo2_profile_text(
	    "plugin.esp\n",
	    "+EnabledMod\n-DisabledMod\n+AnotherEnabled\n");

	REQUIRE(result.enabled_mods.size() == 2);
	REQUIRE(result.enabled_mods[0] == "AnotherEnabled");
	REQUIRE(result.enabled_mods[1] == "EnabledMod");
}

TEST_CASE("profile_reader_t::resolve_single_openmw_content, last data dir wins", "[u]")
{
	std::set<std::string> existing = {
	    "/data1/Morrowind.esm",
	    "/data2/Morrowind.esm",
	};

	auto file_exists = [&](const std::string & path) { return existing.count(path) > 0; };

	const auto result = profile_reader_t::resolve_single_openmw_content(
	    "Morrowind.esm",
	    { "/data1", "/data2" },
	    file_exists);

	REQUIRE(result == "/data2/Morrowind.esm");
}

TEST_CASE("profile_reader_t::resolve_single_openmw_content, falls back to earlier dir", "[u]")
{
	std::set<std::string> existing = {
	    "/data1/Morrowind.esm",
	};

	auto file_exists = [&](const std::string & path) { return existing.count(path) > 0; };

	const auto result = profile_reader_t::resolve_single_openmw_content(
	    "Morrowind.esm",
	    { "/data1", "/data2" },
	    file_exists);

	REQUIRE(result == "/data1/Morrowind.esm");
}

TEST_CASE("profile_reader_t::resolve_single_openmw_content, returns empty when not found", "[u]")
{
	auto file_exists = [](const std::string &) { return false; };

	const auto result = profile_reader_t::resolve_single_openmw_content(
	    "Missing.esm",
	    { "/data1", "/data2" },
	    file_exists);

	REQUIRE(result.empty());
}

TEST_CASE("profile_reader_t::resolve_openmw_content, resolves multiple plugins", "[u]")
{
	std::set<std::string> existing = {
	    "/data/Morrowind.esm",
	    "/data/Tribunal.esm",
	};

	auto file_exists = [&](const std::string & path) { return existing.count(path) > 0; };

	const auto result = profile_reader_t::resolve_openmw_content(
	    { "Morrowind.esm", "Tribunal.esm", "Missing.esp" },
	    { "/data" },
	    file_exists);

	REQUIRE(result.size() == 2);
	REQUIRE(result[0] == "/data/Morrowind.esm");
	REQUIRE(result[1] == "/data/Tribunal.esm");
}

TEST_CASE("profile_reader_t::resolve_mo2_plugin, overwrite has highest priority", "[u]")
{
	std::set<std::string> existing = {
	    "/overwrite/plugin.esp",
	    "/mods/ModA/plugin.esp",
	    "/game/Data Files/plugin.esp",
	};

	auto file_exists = [&](const std::string & path) { return existing.count(path) > 0; };

	const auto result = profile_reader_t::resolve_mo2_plugin(
	    "plugin.esp",
	    "/overwrite",
	    { "ModA" },
	    "/mods",
	    "/game/Data Files",
	    file_exists);

	REQUIRE(result == "/overwrite/plugin.esp");
}

TEST_CASE("profile_reader_t::resolve_mo2_plugin, falls back to mods", "[u]")
{
	std::set<std::string> existing = {
	    "/mods/ModA/plugin.esp",
	    "/game/Data Files/plugin.esp",
	};

	auto file_exists = [&](const std::string & path) { return existing.count(path) > 0; };

	const auto result = profile_reader_t::resolve_mo2_plugin(
	    "plugin.esp",
	    "/overwrite",
	    { "ModA" },
	    "/mods",
	    "/game/Data Files",
	    file_exists);

	REQUIRE(result == "/mods/ModA/plugin.esp");
}

TEST_CASE("profile_reader_t::resolve_mo2_plugin, first mod in list wins", "[u]")
{
	std::set<std::string> existing = {
	    "/mods/ModA/plugin.esp",
	    "/mods/ModB/plugin.esp",
	};

	auto file_exists = [&](const std::string & path) { return existing.count(path) > 0; };

	const auto result = profile_reader_t::resolve_mo2_plugin(
	    "plugin.esp",
	    "/overwrite",
	    { "ModA", "ModB" },
	    "/mods",
	    "/game/Data Files",
	    file_exists);

	REQUIRE(result == "/mods/ModA/plugin.esp");
}

TEST_CASE("profile_reader_t::resolve_mo2_plugin, falls back to game data", "[u]")
{
	std::set<std::string> existing = {
	    "/game/Data Files/Morrowind.esm",
	};

	auto file_exists = [&](const std::string & path) { return existing.count(path) > 0; };

	const auto result = profile_reader_t::resolve_mo2_plugin(
	    "Morrowind.esm",
	    "/overwrite",
	    { "ModA" },
	    "/mods",
	    "/game/Data Files",
	    file_exists);

	REQUIRE(result == "/game/Data Files/Morrowind.esm");
}

TEST_CASE("profile_reader_t::resolve_mo2_plugin, returns empty when not found", "[u]")
{
	auto file_exists = [](const std::string &) { return false; };

	const auto result = profile_reader_t::resolve_mo2_plugin(
	    "missing.esp",
	    "/overwrite",
	    { "ModA" },
	    "/mods",
	    "/game/Data Files",
	    file_exists);

	REQUIRE(result.empty());
}

TEST_CASE("profile_reader_t::resolve_merge_output_path, folder mode", "[u]")
{
	const auto result = profile_reader_t::resolve_merge_output_path(
	    load_source_kind_t::folder,
	    "C:/Games/Morrowind/Data Files");

	REQUIRE(result == "C:/Games/Morrowind/Data Files/Merged Patch.esp");
}

TEST_CASE("profile_reader_t::resolve_merge_output_path, mo2 profile mode", "[u]")
{
	const auto result = profile_reader_t::resolve_merge_output_path(
	    load_source_kind_t::mo2_profile,
	    "C:/MO2/profiles/Default");

	REQUIRE(result == "C:/MO2/profiles/Default/../../overwrite/Merged Patch.esp");
}

TEST_CASE("profile_reader_t::resolve_merge_output_path, openmw cfg mode", "[u]")
{
	const auto result = profile_reader_t::resolve_merge_output_path(
	    load_source_kind_t::openmw_cfg,
	    "/home/user/.config/openmw");

	REQUIRE(result == "/home/user/.config/openmw/data/Merged Patch.esp");
}

TEST_CASE("profile_reader_t::resolve_merge_output_path, empty base returns empty", "[u]")
{
	const auto result = profile_reader_t::resolve_merge_output_path(
	    load_source_kind_t::folder, "");

	REQUIRE(result.empty());
}

#include <QDir>
#include <QFile>
#include <QString>
#include <filesystem>
#include <fstream>

TEST_CASE("profile_reader_t, mo2 merge output path resolves via QDir", "[u]")
{
	auto result = QDir::cleanPath("C:/MO2/profiles/Default/../../overwrite/Merged Patch.esp").toStdString();

	REQUIRE(result.find("overwrite") != std::string::npos);
	REQUIRE(result.find("Merged Patch.esp") != std::string::npos);
	REQUIRE(result.find("profiles") == std::string::npos);
}

TEST_CASE("profile_reader_t, openmw merge output path resolves via QDir", "[u]")
{
	auto result = QDir::cleanPath("/home/user/.config/openmw/data/Merged Patch.esp").toStdString();

	REQUIRE(result == "/home/user/.config/openmw/data/Merged Patch.esp");
}

TEST_CASE("profile_reader_t, folder merge output path resolves via QDir", "[u]")
{
	auto base = QDir("C:/Games/Morrowind/Data Files");
	auto result = base.filePath("Merged Patch.esp").toStdString();

	REQUIRE(result == "C:/Games/Morrowind/Data Files/Merged Patch.esp");
}

TEST_CASE("profile_reader_t, mo2 overwrite resolves merged patch in load order", "[u]")
{
	std::set<std::string> existing = {
	    "/MO2/overwrite/Merged Patch.esp",
	    "/MO2/mods/ModA/Morrowind.esm",
	    "/game/Data Files/Morrowind.esm",
	};

	auto file_exists = [&](const std::string & path) { return existing.count(path) > 0; };

	auto resolved = profile_reader_t::resolve_mo2_plugin(
	    "Merged Patch.esp",
	    "/MO2/overwrite",
	    { "ModA" },
	    "/MO2/mods",
	    "/game/Data Files",
	    file_exists);

	REQUIRE(resolved == "/MO2/overwrite/Merged Patch.esp");
}

TEST_CASE("profile_reader_t, mo2 merged patch not in overwrite returns empty", "[u]")
{
	std::set<std::string> existing = {
	    "/MO2/mods/ModA/Morrowind.esm",
	};

	auto file_exists = [&](const std::string & path) { return existing.count(path) > 0; };

	auto resolved = profile_reader_t::resolve_mo2_plugin(
	    "Merged Patch.esp",
	    "/MO2/overwrite",
	    { "ModA" },
	    "/MO2/mods",
	    "/game/Data Files",
	    file_exists);

	REQUIRE(resolved.empty());
}

TEST_CASE("profile_reader_t, session restore creates and finds merge patch", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_root = fs::temp_directory_path() / "yampt_test_session";
	const auto data_dir = temp_root / "Data Files";
	fs::create_directories(data_dir);

	std::ofstream(data_dir / "Morrowind.esm", std::ios::binary).put('\0');
	std::ofstream(data_dir / "Merged Patch.esp", std::ios::binary).put('\0');

	auto base_path = data_dir.string();
	std::replace(base_path.begin(), base_path.end(), '\\', '/');

	auto merge_path = profile_reader_t::resolve_merge_output_path(
	    load_source_kind_t::folder, base_path);

	REQUIRE(QFile::exists(QString::fromStdString(merge_path)));

	fs::remove_all(temp_root);
}

TEST_CASE("profile_reader_t, mo2 session restore finds merge in overwrite", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_root = fs::temp_directory_path() / "yampt_test_mo2_session";
	const auto profiles_dir = temp_root / "profiles" / "Default";
	const auto overwrite_dir = temp_root / "overwrite";
	fs::create_directories(profiles_dir);
	fs::create_directories(overwrite_dir);

	std::ofstream(overwrite_dir / "Merged Patch.esp", std::ios::binary).put('\0');

	auto base_path = profiles_dir.string();
	std::replace(base_path.begin(), base_path.end(), '\\', '/');

	auto merge_path = profile_reader_t::resolve_merge_output_path(
	    load_source_kind_t::mo2_profile, base_path);

	REQUIRE(QFile::exists(QString::fromStdString(merge_path)));

	fs::remove_all(temp_root);
}

TEST_CASE("profile_reader_t, openmw session restore finds merge in data", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_root = fs::temp_directory_path() / "yampt_test_openmw_session";
	const auto data_dir = temp_root / "data";
	fs::create_directories(data_dir);

	std::ofstream(data_dir / "Merged Patch.esp", std::ios::binary).put('\0');

	auto base_path = temp_root.string();
	std::replace(base_path.begin(), base_path.end(), '\\', '/');

	auto merge_path = profile_reader_t::resolve_merge_output_path(
	    load_source_kind_t::openmw_cfg, base_path);

	REQUIRE(QFile::exists(QString::fromStdString(merge_path)));

	fs::remove_all(temp_root);
}
