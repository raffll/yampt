#include <catch2/catch_all.hpp>
#include <model/sidebar_model.hpp>
#include <filesystem>
#include <fstream>
#include <session.hpp>

TEST_CASE("derive_display_name, plugin with language tag", "[u]")
{
	file_entry_t entry;
	entry.filename = "Morrowind.esm";
	entry.path = "c:/games/morrowind/Morrowind.esm";
	entry.type = file_type_t::plugin;
	entry.language_tag = "EN";

	const auto & result = derive_display_name(entry, true, false);
	REQUIRE(result == "[EN] Morrowind.esm");
}

TEST_CASE("derive_display_name, base dict unloaded", "[u]")
{
	file_entry_t entry;
	entry.filename = "Morrowind_BASE_EN-PL.json";
	entry.path = "c:/workspace/Morrowind_BASE_EN-PL.json";
	entry.type = file_type_t::base_dict;

	const auto & result = derive_display_name(entry, false, false);
	REQUIRE(result == "[UNLOADED] [BASE] Morrowind_BASE_EN-PL.json");
}

TEST_CASE("derive_display_name, user dict dirty and loaded", "[u]")
{
	file_entry_t entry;
	entry.filename = "my_dict.json";
	entry.path = "c:/workspace/my_dict.json";
	entry.type = file_type_t::user_dict;

	const auto & result = derive_display_name(entry, true, true);
	REQUIRE(result == "* my_dict.json");
}

TEST_CASE("derive_display_name, base dict loaded and dirty", "[u]")
{
	file_entry_t entry;
	entry.filename = "Morrowind_BASE_EN-DE.json";
	entry.path = "c:/workspace/Morrowind_BASE_EN-DE.json";
	entry.type = file_type_t::base_dict;

	const auto & result = derive_display_name(entry, true, true);
	REQUIRE(result == "* [BASE] Morrowind_BASE_EN-DE.json");
}

TEST_CASE("derive_context_menu, plugin entry has full menu", "[u]")
{
	file_entry_t entry;
	entry.filename = "Morrowind.esm";
	entry.path = "c:/games/Morrowind.esm";
	entry.type = file_type_t::plugin;

	const auto & actions = derive_context_menu(entry, true, false);
	REQUIRE(actions.size() == 6);
	REQUIRE(actions[0] == menu_action_t::make_dict);
	REQUIRE(actions[1] == menu_action_t::make_dict_with_base);
	REQUIRE(actions[2] == menu_action_t::make_base);
	REQUIRE(actions[3] == menu_action_t::convert);
	REQUIRE(actions[4] == menu_action_t::create_plugin);
	REQUIRE(actions[5] == menu_action_t::delete_file);
}

TEST_CASE("derive_context_menu, yaml entry has only delete", "[u]")
{
	file_entry_t entry;
	entry.filename = "en.yaml";
	entry.path = "c:/workspace/en.yaml";
	entry.type = file_type_t::yaml_l10n;

	const auto & actions = derive_context_menu(entry, true, false);
	REQUIRE(actions.size() == 1);
	REQUIRE(actions[0] == menu_action_t::delete_file);
}

TEST_CASE("derive_context_menu, loaded dirty dict has save and delete", "[u]")
{
	file_entry_t entry;
	entry.filename = "user.json";
	entry.path = "c:/workspace/user.json";
	entry.type = file_type_t::user_dict;

	const auto & actions = derive_context_menu(entry, true, true);
	REQUIRE(actions.size() == 2);
	REQUIRE(actions[0] == menu_action_t::save);
	REQUIRE(actions[1] == menu_action_t::delete_file);
}

TEST_CASE("derive_context_menu, unloaded dict has only delete", "[u]")
{
	file_entry_t entry;
	entry.filename = "base.json";
	entry.path = "c:/workspace/base.json";
	entry.type = file_type_t::base_dict;

	const auto & actions = derive_context_menu(entry, false, false);
	REQUIRE(actions.size() == 1);
	REQUIRE(actions[0] == menu_action_t::delete_file);
}

TEST_CASE("build_render_model, empty file_list produces empty roots", "[u]")
{
	file_list_t empty_list;
	session_t session(codepage_t::windows_1252);

	const auto & model = build_render_model(empty_list, session, "");
	REQUIRE(model.roots.empty());
	REQUIRE(model.active_path.empty());
}

TEST_CASE("build_render_model, groups files by root correctly", "[i]")
{
	namespace fs = std::filesystem;

	const auto temp_root = fs::temp_directory_path() / "yampt_sidebar_test";
	std::error_code ec;
	fs::create_directories(temp_root, ec);

	const auto file_alpha = temp_root / "alpha.json";
	const auto file_beta = temp_root / "beta.json";
	std::ofstream(file_alpha.string()) << "{}";
	std::ofstream(file_beta.string()) << "{}";

	file_list_t list;
	list.scan_roots({ temp_root.string() });

	session_t session(codepage_t::windows_1252);

	const auto & model = build_render_model(list, session, "");
	REQUIRE(model.roots.size() == 1);
	REQUIRE(model.roots[0].items.size() >= 2);

	fs::remove_all(temp_root, ec);
}
