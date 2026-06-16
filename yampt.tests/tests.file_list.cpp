#include "catch.hpp"
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include "../yampt/file_list.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_map>

TEST_CASE("property: file type classification correctness", "[u]")
{
	rc::prop("classify is deterministic and matches extension rules", []()
	{
		const auto ext = *rc::gen::element(
			std::string(".esm"), std::string(".esp"),
			std::string(".json"), std::string(".xml"),
			std::string(".yaml"));
		const auto prefix = *rc::gen::arbitrary<std::string>();
		const auto path = prefix + ext;

		const auto first = classify(path);
		const auto second = classify(path);
		RC_ASSERT(first == second);

		if (ext == ".esm" || ext == ".esp")
		{
			RC_ASSERT(first == file_type_t::plugin);
		}
		else if (ext == ".yaml")
		{
			RC_ASSERT(first == file_type_t::lua_l10n);
		}
		else if (ext == ".json" || ext == ".xml")
		{
			RC_ASSERT(first == file_type_t::base_dict ||
				first == file_type_t::user_dict);

			if (first == file_type_t::base_dict)
			{
				auto filename = path;
				const auto pos = filename.find_last_of("\\/");
				if (pos != std::string::npos)
					filename = filename.substr(pos + 1);

				std::transform(filename.begin(), filename.end(), filename.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				RC_ASSERT(filename.find("_base_") != std::string::npos);
			}
		}
	});
}

TEST_CASE("property: path-keyed insert/lookup/remove invariant", "[u]")
{
	rc::prop("get returns non-null iff add was last operation for path", []()
	{
		file_list_t list;
		const auto paths = *rc::gen::container<std::vector<std::string>>(
			rc::gen::nonEmpty(rc::gen::arbitrary<std::string>()));
		const auto ops = *rc::gen::container<std::vector<bool>>(
			rc::gen::arbitrary<bool>());

		std::unordered_map<std::string, bool> expected;

		const auto count = std::min(paths.size(), ops.size());
		for (size_t i = 0; i < count; ++i)
		{
			if (ops[i])
			{
				list.add(paths[i]);
				expected[paths[i]] = true;
			}
			else
			{
				list.remove(paths[i]);
				expected[paths[i]] = false;
			}
		}

		for (const auto & [path, should_exist] : expected)
		{
			if (should_exist)
			{
				RC_ASSERT(list.get(path) != nullptr);
				RC_ASSERT(list.contains(path));
			}
			else
			{
				RC_ASSERT(list.get(path) == nullptr);
				RC_ASSERT(!list.contains(path));
			}
		}
	});
}

TEST_CASE("property: language tag detection", "[u]")
{
	rc::prop("detect_language returns correct tag or empty for all inputs", []()
	{
		const auto use_known = *rc::gen::arbitrary<bool>();

		if (use_known)
		{
			const auto filename = *rc::gen::element(
				std::string("Morrowind.esm"),
				std::string("Tribunal.esm"),
				std::string("Bloodmoon.esm"));

			std::vector<std::pair<std::uintmax_t, std::string>> known_sizes;
			if (filename == "Morrowind.esm")
			{
				known_sizes = {
					{79837557, "EN"}, {80640776, "DE"}, {80105097, "PL"},
					{80681814, "FR"}, {79857000, "RU"}};
			}
			else if (filename == "Tribunal.esm")
			{
				known_sizes = {
					{9631798, "EN"}, {9797295, "DE"}, {9658076, "PL"},
					{10015689, "FR"}, {9702000, "RU"}};
			}
			else
			{
				known_sizes = {
					{4565686, "EN"}, {6069165, "DE"}, {4626565, "PL"},
					{4697358, "FR"}, {4625000, "RU"}};
			}

			const auto idx = *rc::gen::inRange<size_t>(0, known_sizes.size());
			const auto & [size, expected_tag] = known_sizes[idx];
			const auto result = detect_language(filename, size);
			RC_ASSERT(result == expected_tag);
		}
		else
		{
			const auto filename = *rc::gen::arbitrary<std::string>();
			const auto size = *rc::gen::arbitrary<std::uintmax_t>();
			const auto result = detect_language(filename, size);
			RC_ASSERT(result.empty() || result == "EN" || result == "DE" ||
				result == "PL" || result == "FR" || result == "RU");
		}
	});
}

TEST_CASE("property: display name derivation", "[u]")
{
	rc::prop("display name follows prefix/tag/suffix structure", []()
	{
		file_entry_t entry;
		entry.type = static_cast<file_type_t>(*rc::gen::inRange(0, 4));
		entry.dirty = *rc::gen::arbitrary<bool>();
		entry.filename = *rc::gen::nonEmpty(rc::gen::arbitrary<std::string>());
		entry.language_tag = *rc::gen::element(
			std::string(""), std::string("EN"), std::string("PL"),
			std::string("DE"), std::string("FR"));

		const auto result = derive_display_name(entry);

		if (entry.dirty)
		{
			RC_ASSERT(result.substr(0, 2) == "* ");
		}
		else
		{
			RC_ASSERT(result.substr(0, 2) != "* ");
		}

		std::string type_tag;
		switch (entry.type)
		{
		case file_type_t::plugin: type_tag = "[ESP]"; break;
		case file_type_t::base_dict: type_tag = "[BASE]"; break;
		case file_type_t::user_dict: type_tag = "[USER]"; break;
		case file_type_t::lua_l10n: type_tag = "[LUA]"; break;
		}
		RC_ASSERT(result.find(type_tag) != std::string::npos);

		if (entry.type == file_type_t::plugin && !entry.language_tag.empty())
		{
			const auto lang_marker = "[" + entry.language_tag + "]";
			RC_ASSERT(result.find(lang_marker) != std::string::npos);
		}

		const auto suffix = " " + entry.filename;
		RC_ASSERT(result.size() >= suffix.size());
		RC_ASSERT(result.substr(result.size() - suffix.size()) == suffix);
	});
}

TEST_CASE("property: context menu derivation", "[u]")
{
	rc::prop("menu items match decision table for all valid states", []()
	{
		file_entry_t entry;
		entry.type = static_cast<file_type_t>(*rc::gen::inRange(0, 4));
		entry.loaded = *rc::gen::arbitrary<bool>();
		entry.is_workspace = *rc::gen::arbitrary<bool>();
		entry.dirty = *rc::gen::arbitrary<bool>();
		entry.filename = "test.esp";
		entry.path = "/some/path/test.esp";

		const auto menu = derive_context_menu(entry);

		if (entry.type == file_type_t::lua_l10n)
		{
			RC_ASSERT(menu.size() == 1);
			RC_ASSERT(menu[0] == menu_action_t::delete_file);
			return;
		}

		if (entry.type == file_type_t::plugin)
		{
			if (entry.loaded && !entry.is_workspace)
			{
				RC_ASSERT(menu.size() == 6);
				RC_ASSERT(menu[0] == menu_action_t::make_dict);
				RC_ASSERT(menu[5] == menu_action_t::unload);
			}
			else if (!entry.loaded && entry.is_workspace)
			{
				RC_ASSERT(menu.size() == 6);
				RC_ASSERT(menu[0] == menu_action_t::make_dict);
				RC_ASSERT(menu[5] == menu_action_t::delete_file);
			}
			else
			{
				RC_ASSERT(menu.empty());
			}
			return;
		}

		if (entry.type == file_type_t::base_dict || entry.type == file_type_t::user_dict)
		{
			if (entry.loaded && !entry.is_workspace)
			{
				RC_ASSERT(menu.size() == 3);
				RC_ASSERT(menu[0] == menu_action_t::save);
				RC_ASSERT(menu[1] == menu_action_t::save_as);
				RC_ASSERT(menu[2] == menu_action_t::unload);
			}
			else if (entry.loaded && entry.is_workspace && entry.dirty)
			{
				RC_ASSERT(menu.size() == 2);
				RC_ASSERT(menu[0] == menu_action_t::save);
				RC_ASSERT(menu[1] == menu_action_t::delete_file);
			}
			else if (entry.loaded && entry.is_workspace && !entry.dirty)
			{
				RC_ASSERT(menu.size() == 1);
				RC_ASSERT(menu[0] == menu_action_t::delete_file);
			}
			else if (!entry.loaded && entry.is_workspace)
			{
				RC_ASSERT(menu.size() == 1);
				RC_ASSERT(menu[0] == menu_action_t::delete_file);
			}
			else
			{
				RC_ASSERT(menu.empty());
			}
		}
	});
}

TEST_CASE("property: persistence filter", "[u]")
{
	rc::prop("paths_to_persist contains only loaded non-workspace paths", []()
	{
		file_list_t list;
		const auto count = *rc::gen::inRange(1, 20);

		std::vector<std::string> expected_paths;

		for (int i = 0; i < count; ++i)
		{
			const auto path = "path_" + std::to_string(i) + ".esp";
			auto & entry = list.add(path);
			entry.loaded = *rc::gen::arbitrary<bool>();
			entry.is_workspace = *rc::gen::arbitrary<bool>();

			if (entry.loaded && !entry.is_workspace)
				expected_paths.push_back(path);
		}

		const auto result = list.paths_to_persist();

		std::set<std::string> result_set(result.begin(), result.end());
		std::set<std::string> expected_set(expected_paths.begin(), expected_paths.end());

		RC_ASSERT(result_set == expected_set);
	});
}

TEST_CASE("property: output directory derivation", "[u]")
{
	rc::prop("workspace uses parent dir, non-workspace uses default_dir", []()
	{
		file_entry_t entry;
		entry.is_workspace = *rc::gen::arbitrary<bool>();
		entry.path = "C:/workspace/subfolder/file.esp";
		const auto default_dir = *rc::gen::nonEmpty(rc::gen::arbitrary<std::string>());

		const auto result = derive_output_dir(entry, default_dir);

		if (entry.is_workspace)
		{
			RC_ASSERT(result == "C:/workspace/subfolder");
		}
		else
		{
			RC_ASSERT(result == default_dir);
		}
	});
}

TEST_CASE("property: dirty flag round-trip", "[u]")
{
	rc::prop("set_dirty affects only the targeted entry", []()
	{
		file_list_t list;
		const auto count = *rc::gen::inRange(2, 10);
		std::vector<std::string> paths;

		for (int i = 0; i < count; ++i)
		{
			const auto path = "entry_" + std::to_string(i) + ".json";
			paths.push_back(path);
			auto & entry = list.add(path);
			entry.loaded = true;
		}

		const auto target_idx = *rc::gen::inRange(0, count);
		const auto dirty_val = *rc::gen::arbitrary<bool>();

		list.set_dirty(paths[target_idx], dirty_val);

		for (int i = 0; i < count; ++i)
		{
			const auto * entry = list.get(paths[i]);
			if (i == target_idx)
			{
				RC_ASSERT(entry->dirty == dirty_val);
			}
			else
			{
				RC_ASSERT(entry->dirty == false);
			}
		}
	});
}

TEST_CASE("classify edge cases", "[u]")
{
	SECTION("mixed case extensions")
	{
		REQUIRE(classify("C:/path/Morrowind.ESM") == file_type_t::plugin);
		REQUIRE(classify("C:/path/plugin.Esp") == file_type_t::plugin);
		REQUIRE(classify("C:/path/dict.JSON") == file_type_t::user_dict);
		REQUIRE(classify("C:/path/l10n.YAML") == file_type_t::lua_l10n);
	}

	SECTION("paths with multiple dots")
	{
		REQUIRE(classify("C:/path/my.plugin.esp") == file_type_t::plugin);
		REQUIRE(classify("C:/path/some.dict.json") == file_type_t::user_dict);
	}

	SECTION("_BASE_ pattern in filename vs directory")
	{
		REQUIRE(classify("C:/path/Morrowind_BASE_EN-PL.json") == file_type_t::base_dict);
		REQUIRE(classify("C:/_BASE_/dict.json") == file_type_t::user_dict);
		REQUIRE(classify("C:/path/dict_BASE_stuff.xml") == file_type_t::base_dict);
	}
}

TEST_CASE("detect_language examples", "[u]")
{
	SECTION("morrowind.esm known sizes")
	{
		REQUIRE(detect_language("Morrowind.esm", 79837557) == "EN");
		REQUIRE(detect_language("Morrowind.esm", 80640776) == "DE");
		REQUIRE(detect_language("Morrowind.esm", 80105097) == "PL");
		REQUIRE(detect_language("Morrowind.esm", 80681814) == "FR");
		REQUIRE(detect_language("Morrowind.esm", 79857000) == "RU");
	}

	SECTION("tribunal.esm known sizes")
	{
		REQUIRE(detect_language("Tribunal.esm", 9631798) == "EN");
		REQUIRE(detect_language("Tribunal.esm", 9797295) == "DE");
		REQUIRE(detect_language("Tribunal.esm", 9658076) == "PL");
		REQUIRE(detect_language("Tribunal.esm", 10015689) == "FR");
		REQUIRE(detect_language("Tribunal.esm", 9702000) == "RU");
	}

	SECTION("bloodmoon.esm known sizes")
	{
		REQUIRE(detect_language("Bloodmoon.esm", 4565686) == "EN");
		REQUIRE(detect_language("Bloodmoon.esm", 6069165) == "DE");
		REQUIRE(detect_language("Bloodmoon.esm", 4626565) == "PL");
		REQUIRE(detect_language("Bloodmoon.esm", 4697358) == "FR");
		REQUIRE(detect_language("Bloodmoon.esm", 4625000) == "RU");
	}

	SECTION("unknown filename with known size returns empty")
	{
		REQUIRE(detect_language("Unknown.esm", 79837557) == "");
		REQUIRE(detect_language("CustomPlugin.esp", 9631798) == "");
	}

	SECTION("known filename with unknown size returns empty")
	{
		REQUIRE(detect_language("Morrowind.esm", 12345) == "");
		REQUIRE(detect_language("Tribunal.esm", 99999999) == "");
		REQUIRE(detect_language("Bloodmoon.esm", 0) == "");
	}
}

TEST_CASE("derive_display_name examples", "[u]")
{
	SECTION("dirty user dict")
	{
		file_entry_t entry;
		entry.type = file_type_t::user_dict;
		entry.dirty = true;
		entry.filename = "Morrowind_en.json";
		entry.language_tag = "";
		REQUIRE(derive_display_name(entry) == "* [USER] Morrowind_en.json");
	}

	SECTION("clean plugin with language tag")
	{
		file_entry_t entry;
		entry.type = file_type_t::plugin;
		entry.dirty = false;
		entry.filename = "Morrowind.esm";
		entry.language_tag = "EN";
		REQUIRE(derive_display_name(entry) == "[ESP] [EN] Morrowind.esm");
	}

	SECTION("base dict")
	{
		file_entry_t entry;
		entry.type = file_type_t::base_dict;
		entry.dirty = false;
		entry.filename = "Morrowind_BASE_EN-PL.json";
		entry.language_tag = "";
		REQUIRE(derive_display_name(entry) == "[BASE] Morrowind_BASE_EN-PL.json");
	}

	SECTION("lua l10n")
	{
		file_entry_t entry;
		entry.type = file_type_t::lua_l10n;
		entry.dirty = false;
		entry.filename = "en.yaml";
		entry.language_tag = "";
		REQUIRE(derive_display_name(entry) == "[LUA] en.yaml");
	}
}

TEST_CASE("derive_context_menu decision table", "[u]")
{
	SECTION("plugin loaded non-workspace")
	{
		file_entry_t entry;
		entry.type = file_type_t::plugin;
		entry.loaded = true;
		entry.is_workspace = false;
		entry.dirty = false;
		const auto menu = derive_context_menu(entry);
		REQUIRE(menu.size() == 6);
		REQUIRE(menu[0] == menu_action_t::make_dict);
		REQUIRE(menu[1] == menu_action_t::make_dict_with_base);
		REQUIRE(menu[2] == menu_action_t::make_base);
		REQUIRE(menu[3] == menu_action_t::convert);
		REQUIRE(menu[4] == menu_action_t::create_plugin);
		REQUIRE(menu[5] == menu_action_t::unload);
	}

	SECTION("plugin not loaded workspace")
	{
		file_entry_t entry;
		entry.type = file_type_t::plugin;
		entry.loaded = false;
		entry.is_workspace = true;
		entry.dirty = false;
		const auto menu = derive_context_menu(entry);
		REQUIRE(menu.size() == 6);
		REQUIRE(menu[0] == menu_action_t::make_dict);
		REQUIRE(menu[1] == menu_action_t::make_dict_with_base);
		REQUIRE(menu[2] == menu_action_t::make_base);
		REQUIRE(menu[3] == menu_action_t::convert);
		REQUIRE(menu[4] == menu_action_t::create_plugin);
		REQUIRE(menu[5] == menu_action_t::delete_file);
	}

	SECTION("base_dict loaded non-workspace")
	{
		file_entry_t entry;
		entry.type = file_type_t::base_dict;
		entry.loaded = true;
		entry.is_workspace = false;
		entry.dirty = false;
		const auto menu = derive_context_menu(entry);
		REQUIRE(menu.size() == 3);
		REQUIRE(menu[0] == menu_action_t::save);
		REQUIRE(menu[1] == menu_action_t::save_as);
		REQUIRE(menu[2] == menu_action_t::unload);
	}

	SECTION("user_dict loaded workspace dirty")
	{
		file_entry_t entry;
		entry.type = file_type_t::user_dict;
		entry.loaded = true;
		entry.is_workspace = true;
		entry.dirty = true;
		const auto menu = derive_context_menu(entry);
		REQUIRE(menu.size() == 2);
		REQUIRE(menu[0] == menu_action_t::save);
		REQUIRE(menu[1] == menu_action_t::delete_file);
	}

	SECTION("base_dict loaded workspace not dirty")
	{
		file_entry_t entry;
		entry.type = file_type_t::base_dict;
		entry.loaded = true;
		entry.is_workspace = true;
		entry.dirty = false;
		const auto menu = derive_context_menu(entry);
		REQUIRE(menu.size() == 1);
		REQUIRE(menu[0] == menu_action_t::delete_file);
	}

	SECTION("user_dict not loaded workspace")
	{
		file_entry_t entry;
		entry.type = file_type_t::user_dict;
		entry.loaded = false;
		entry.is_workspace = true;
		entry.dirty = false;
		const auto menu = derive_context_menu(entry);
		REQUIRE(menu.size() == 1);
		REQUIRE(menu[0] == menu_action_t::delete_file);
	}

	SECTION("lua_l10n any state")
	{
		file_entry_t entry;
		entry.type = file_type_t::lua_l10n;
		entry.loaded = true;
		entry.is_workspace = true;
		entry.dirty = true;
		const auto menu = derive_context_menu(entry);
		REQUIRE(menu.size() == 1);
		REQUIRE(menu[0] == menu_action_t::delete_file);
	}
}

TEST_CASE("file_list_t container operations", "[u]")
{
	file_list_t list;

	SECTION("add and get")
	{
		auto & entry = list.add("C:/path/file.esp");
		REQUIRE(entry.path == "C:/path/file.esp");
		REQUIRE(entry.filename == "file.esp");
		REQUIRE(entry.type == file_type_t::plugin);

		const auto * found = list.get("C:/path/file.esp");
		REQUIRE(found != nullptr);
		REQUIRE(found->path == "C:/path/file.esp");
	}

	SECTION("remove")
	{
		list.add("C:/path/file.esp");
		REQUIRE(list.contains("C:/path/file.esp"));
		list.remove("C:/path/file.esp");
		REQUIRE(!list.contains("C:/path/file.esp"));
		REQUIRE(list.get("C:/path/file.esp") == nullptr);
	}

	SECTION("contains")
	{
		REQUIRE(!list.contains("C:/nonexistent.esp"));
		list.add("C:/nonexistent.esp");
		REQUIRE(list.contains("C:/nonexistent.esp"));
	}

	SECTION("duplicate add returns existing entry")
	{
		auto & first = list.add("C:/path/file.esp");
		first.loaded = true;
		auto & second = list.add("C:/path/file.esp");
		REQUIRE(second.loaded == true);
		REQUIRE(&first == &second);
	}

	SECTION("set_loaded")
	{
		list.add("C:/path/file.esp");
		list.set_loaded("C:/path/file.esp", true);
		REQUIRE(list.get("C:/path/file.esp")->loaded == true);
		list.set_loaded("C:/path/file.esp", false);
		REQUIRE(list.get("C:/path/file.esp")->loaded == false);
	}

	SECTION("set_dirty no-op on unloaded entry")
	{
		list.add("C:/path/file.json");
		list.set_dirty("C:/path/file.json", true);
		REQUIRE(list.get("C:/path/file.json")->dirty == false);
	}

	SECTION("set_dirty works on loaded entry")
	{
		auto & entry = list.add("C:/path/file.json");
		entry.loaded = true;
		list.set_dirty("C:/path/file.json", true);
		REQUIRE(list.get("C:/path/file.json")->dirty == true);
	}

	SECTION("set_loaded false clears dirty")
	{
		auto & entry = list.add("C:/path/file.json");
		entry.loaded = true;
		entry.dirty = true;
		list.set_loaded("C:/path/file.json", false);
		REQUIRE(list.get("C:/path/file.json")->dirty == false);
	}
}

TEST_CASE("property: section grouping", "[u]")
{
	rc::prop("build_render_model groups entries correctly", []()
	{
		file_list_t list;
		const auto count = *rc::gen::inRange(1, 30);

		struct expected_entry_t
		{
			std::string path;
			std::string display_text;
			bool loaded_non_workspace;
			bool workspace_root_file;
			std::string subfolder;
		};

		std::vector<expected_entry_t> expected;

		for (int i = 0; i < count; ++i)
		{
			const auto ext = *rc::gen::element(
				std::string(".esp"), std::string(".json"),
				std::string(".xml"), std::string(".yaml"));
			const auto path = "C:/workspace/" + std::to_string(i) + "_file" + ext;

			auto & entry = list.add(path);
			entry.loaded = *rc::gen::arbitrary<bool>();
			entry.is_workspace = *rc::gen::arbitrary<bool>();
			entry.dirty = *rc::gen::arbitrary<bool>();

			if (entry.is_workspace)
			{
				const auto use_subfolder = *rc::gen::arbitrary<bool>();
				if (use_subfolder)
				{
					entry.workspace_subfolder = *rc::gen::element(
						std::string("en"), std::string("pl"),
						std::string("de"), std::string("fr"));
				}
			}

			expected_entry_t exp;
			exp.path = entry.path;
			exp.display_text = derive_display_name(entry);
			exp.loaded_non_workspace = entry.loaded && !entry.is_workspace;
			exp.workspace_root_file = entry.is_workspace && entry.workspace_subfolder.empty();
			exp.subfolder = entry.is_workspace ? entry.workspace_subfolder : "";
			expected.push_back(std::move(exp));
		}

		const auto model = build_render_model(list, "");

		std::set<std::string> loaded_paths;
		for (const auto & item : model.loaded_root.items)
			loaded_paths.insert(item.path);

		std::set<std::string> expected_loaded_paths;
		for (const auto & e : expected)
		{
			if (e.loaded_non_workspace)
				expected_loaded_paths.insert(e.path);
		}
		RC_ASSERT(loaded_paths == expected_loaded_paths);

		std::set<std::string> ws_root_paths;
		for (const auto & item : model.workspace_root.items)
			ws_root_paths.insert(item.path);

		std::set<std::string> expected_ws_root_paths;
		for (const auto & e : expected)
		{
			if (e.workspace_root_file)
				expected_ws_root_paths.insert(e.path);
		}
		RC_ASSERT(ws_root_paths == expected_ws_root_paths);

		std::set<std::string> expected_subfolders;
		for (const auto & e : expected)
		{
			if (e.subfolder.empty())
				continue;

			expected_subfolders.insert(e.subfolder);
		}

		RC_ASSERT(model.workspace_root.children.size() == expected_subfolders.size());

		for (const auto & child : model.workspace_root.children)
		{
			const auto subfolder = child.label.substr(0, child.label.size() - 1);
			RC_ASSERT(expected_subfolders.count(subfolder) == 1);

			std::set<std::string> child_paths;
			for (const auto & item : child.items)
				child_paths.insert(item.path);

			std::set<std::string> expected_child_paths;
			for (const auto & e : expected)
			{
				if (e.subfolder == subfolder)
					expected_child_paths.insert(e.path);
			}
			RC_ASSERT(child_paths == expected_child_paths);

			for (size_t i = 1; i < child.items.size(); ++i)
				RC_ASSERT(child.items[i - 1].display_text <= child.items[i].display_text);
		}

		for (size_t i = 1; i < model.loaded_root.items.size(); ++i)
			RC_ASSERT(model.loaded_root.items[i - 1].display_text <= model.loaded_root.items[i].display_text);

		for (size_t i = 1; i < model.workspace_root.items.size(); ++i)
			RC_ASSERT(model.workspace_root.items[i - 1].display_text <= model.workspace_root.items[i].display_text);
	});
}

TEST_CASE("workspace scan round-trip", "[i]")
{
	namespace fs = std::filesystem;

	const auto temp_dir = fs::temp_directory_path() / "yampt_test_workspace";
	fs::remove_all(temp_dir);
	fs::create_directories(temp_dir);
	fs::create_directories(temp_dir / "en");
	fs::create_directories(temp_dir / "pl");

	std::ofstream(temp_dir / "test.esp").put('x');
	std::ofstream(temp_dir / "dict.json").put('x');
	std::ofstream(temp_dir / "data_BASE_EN.json").put('x');
	std::ofstream(temp_dir / "en" / "morrowind.json").put('x');
	std::ofstream(temp_dir / "pl" / "morrowind_pl.json").put('x');

	file_list_t list;
	list.scan_workspace(temp_dir.string());

	REQUIRE(list.contains((temp_dir / "test.esp").string()));
	REQUIRE(list.contains((temp_dir / "dict.json").string()));
	REQUIRE(list.contains((temp_dir / "data_BASE_EN.json").string()));
	REQUIRE(list.contains((temp_dir / "en" / "morrowind.json").string()));
	REQUIRE(list.contains((temp_dir / "pl" / "morrowind_pl.json").string()));

	const auto * root_esp = list.get((temp_dir / "test.esp").string());
	REQUIRE(root_esp != nullptr);
	REQUIRE(root_esp->is_workspace == true);
	REQUIRE(root_esp->workspace_subfolder.empty());
	REQUIRE(root_esp->type == file_type_t::plugin);

	const auto * root_base = list.get((temp_dir / "data_BASE_EN.json").string());
	REQUIRE(root_base != nullptr);
	REQUIRE(root_base->is_workspace == true);
	REQUIRE(root_base->workspace_subfolder.empty());
	REQUIRE(root_base->type == file_type_t::base_dict);

	const auto * sub_en = list.get((temp_dir / "en" / "morrowind.json").string());
	REQUIRE(sub_en != nullptr);
	REQUIRE(sub_en->is_workspace == true);
	REQUIRE(sub_en->workspace_subfolder == "en");
	REQUIRE(sub_en->type == file_type_t::user_dict);

	const auto * sub_pl = list.get((temp_dir / "pl" / "morrowind_pl.json").string());
	REQUIRE(sub_pl != nullptr);
	REQUIRE(sub_pl->is_workspace == true);
	REQUIRE(sub_pl->workspace_subfolder == "pl");
	REQUIRE(sub_pl->type == file_type_t::user_dict);

	const auto all = list.all();
	REQUIRE(all.size() == 5);
	for (const auto * entry : all)
		REQUIRE(entry->is_workspace == true);

	fs::remove_all(temp_dir);
}

TEST_CASE("config persistence round-trip via file_list paths_to_persist", "[i]")
{
	file_list_t list;

	auto & loaded_plugin = list.add("C:/loaded/Morrowind.esm");
	loaded_plugin.loaded = true;
	loaded_plugin.is_workspace = false;

	auto & loaded_dict = list.add("C:/loaded/dict_en.json");
	loaded_dict.loaded = true;
	loaded_dict.is_workspace = false;

	auto & workspace_file = list.add("C:/workspace/test.esp");
	workspace_file.loaded = false;
	workspace_file.is_workspace = true;

	auto & workspace_loaded = list.add("C:/workspace/loaded.json");
	workspace_loaded.loaded = true;
	workspace_loaded.is_workspace = true;

	auto & unloaded = list.add("C:/other/unused.json");
	unloaded.loaded = false;
	unloaded.is_workspace = false;

	const auto persisted = list.paths_to_persist();

	REQUIRE(persisted.size() == 2);
	std::set<std::string> persisted_set(persisted.begin(), persisted.end());
	REQUIRE(persisted_set.count("C:/loaded/Morrowind.esm") == 1);
	REQUIRE(persisted_set.count("C:/loaded/dict_en.json") == 1);
	REQUIRE(persisted_set.count("C:/workspace/test.esp") == 0);
	REQUIRE(persisted_set.count("C:/workspace/loaded.json") == 0);
	REQUIRE(persisted_set.count("C:/other/unused.json") == 0);

	file_list_t restored;
	for (const auto & path : persisted)
	{
		auto & entry = restored.add(path);
		entry.loaded = true;
	}

	REQUIRE(restored.contains("C:/loaded/Morrowind.esm"));
	REQUIRE(restored.contains("C:/loaded/dict_en.json"));
	REQUIRE(!restored.contains("C:/workspace/test.esp"));
	REQUIRE(!restored.contains("C:/workspace/loaded.json"));
	REQUIRE(!restored.contains("C:/other/unused.json"));

	const auto restored_persist = restored.paths_to_persist();
	std::set<std::string> restored_set(restored_persist.begin(), restored_persist.end());
	REQUIRE(restored_set == persisted_set);
}
