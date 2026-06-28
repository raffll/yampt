#include <catch2/catch_all.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include "../yampt/io/file_list.hpp"
#include "../yampt.translator/model/sidebar_model.hpp"
#include "../yampt.translator/session.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_map>

TEST_CASE("file_list_t::classify, correctness", "[u]")
{
	rc::prop(
	    "classify is deterministic and matches extension rules",
	    []()
	{
		const auto ext = *rc::gen::element(
		    std::string(".esm"), std::string(".esp"), std::string(".json"), std::string(".xml"), std::string(".yaml"));
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
			RC_ASSERT(first == file_type_t::yaml_l10n);
		}
		else if (ext == ".json" || ext == ".xml")
		{
			RC_ASSERT(first == file_type_t::base_dict || first == file_type_t::user_dict);

			if (first == file_type_t::base_dict)
			{
				auto filename = path;
				const auto pos = filename.find_last_of("\\/");
				if (pos != std::string::npos)
					filename = filename.substr(pos + 1);

				std::transform(
				    filename.begin(),
				    filename.end(),
				    filename.begin(),
				    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				RC_ASSERT(filename.find("_base_") != std::string::npos);
			}
		}
	});
}

TEST_CASE("file_list_t, insert/lookup/remove invariant", "[u]")
{
	rc::prop(
	    "get returns non-null iff add was last operation for path",
	    []()
	{
		file_list_t list;
		const auto paths =
		    *rc::gen::container<std::vector<std::string>>(rc::gen::nonEmpty(rc::gen::arbitrary<std::string>()));
		const auto ops = *rc::gen::container<std::vector<bool>>(rc::gen::arbitrary<bool>());

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

TEST_CASE("file_list_t::detect_language, known and unknown inputs", "[u]")
{
	rc::prop(
	    "detect_language returns correct tag or empty for all inputs",
	    []()
	{
		const auto use_known = *rc::gen::arbitrary<bool>();

		if (use_known)
		{
			const auto filename = *rc::gen::element(
			    std::string("Morrowind.esm"), std::string("Tribunal.esm"), std::string("Bloodmoon.esm"));

			std::vector<std::pair<std::uintmax_t, std::string>> known_sizes;
			if (filename == "Morrowind.esm")
			{
				known_sizes = {
					{ 79837557, "EN" }, { 80640776, "DE" }, { 80105097, "PL" }, { 80681814, "FR" }, { 79857000, "RU" }
				};
			}
			else if (filename == "Tribunal.esm")
			{
				known_sizes = {
					{ 9631798, "EN" }, { 9797295, "DE" }, { 9658076, "PL" }, { 10015689, "FR" }, { 9702000, "RU" }
				};
			}
			else
			{
				known_sizes = {
					{ 4565686, "EN" }, { 6069165, "DE" }, { 4626565, "PL" }, { 4697358, "FR" }, { 4625000, "RU" }
				};
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
			RC_ASSERT(
			    result.empty() || result == "EN" || result == "DE" || result == "PL" || result == "FR" ||
			    result == "RU");
		}
	});
}

TEST_CASE("file_list_t::derive_context_menu, decision table property", "[u]")
{
	rc::prop(
	    "menu items match decision table for all valid states",
	    []()
	{
		file_entry_t entry;
		entry.type = static_cast<file_type_t>(*rc::gen::inRange(0, 4));
		const auto is_loaded = *rc::gen::arbitrary<bool>();
		const auto is_dirty = *rc::gen::arbitrary<bool>();
		entry.is_workspace = true;
		entry.filename = "test.esp";
		entry.path = "/some/path/test.esp";

		const auto menu = derive_context_menu(entry, is_loaded, is_dirty);

		if (entry.type == file_type_t::yaml_l10n)
		{
			RC_ASSERT(menu.size() == 1);
			RC_ASSERT(menu[0] == menu_action_t::delete_file);
			return;
		}

		if (entry.type == file_type_t::plugin)
		{
			RC_ASSERT(menu.size() == 6);
			RC_ASSERT(menu[0] == menu_action_t::make_dict);
			RC_ASSERT(menu[5] == menu_action_t::delete_file);
			return;
		}

		if (entry.type == file_type_t::base_dict || entry.type == file_type_t::user_dict)
		{
			if (is_loaded && is_dirty)
			{
				RC_ASSERT(menu.size() == 2);
				RC_ASSERT(menu[0] == menu_action_t::save);
				RC_ASSERT(menu[1] == menu_action_t::delete_file);
			}
			else
			{
				RC_ASSERT(menu.size() == 1);
				RC_ASSERT(menu[0] == menu_action_t::delete_file);
			}
		}
	});
}

TEST_CASE("file_list_t::derive_output_dir, workspace vs non-workspace", "[u]")
{
	rc::prop(
	    "workspace uses parent dir, non-workspace uses default_dir",
	    []()
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

TEST_CASE("file_list_t::classify, edge cases", "[u]")
{
	SECTION("mixed case extensions")
	{
		REQUIRE(classify("C:/path/Morrowind.ESM") == file_type_t::plugin);
		REQUIRE(classify("C:/path/plugin.Esp") == file_type_t::plugin);
		REQUIRE(classify("C:/path/dict.JSON") == file_type_t::user_dict);
		REQUIRE(classify("C:/path/l10n.YAML") == file_type_t::yaml_l10n);
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

TEST_CASE("file_list_t::detect_language, examples", "[u]")
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

TEST_CASE("file_list_t::derive_context_menu, decision table", "[u]")
{
	SECTION("plugin workspace")
	{
		file_entry_t entry;
		entry.type = file_type_t::plugin;
		entry.is_workspace = true;
		const auto menu = derive_context_menu(entry, false, false);
		REQUIRE(menu.size() == 6);
		REQUIRE(menu[0] == menu_action_t::make_dict);
		REQUIRE(menu[1] == menu_action_t::make_dict_with_base);
		REQUIRE(menu[2] == menu_action_t::make_base);
		REQUIRE(menu[3] == menu_action_t::convert);
		REQUIRE(menu[4] == menu_action_t::create_plugin);
		REQUIRE(menu[5] == menu_action_t::delete_file);
	}

	SECTION("user_dict loaded workspace dirty")
	{
		file_entry_t entry;
		entry.type = file_type_t::user_dict;
		entry.is_workspace = true;
		const auto menu = derive_context_menu(entry, true, true);
		REQUIRE(menu.size() == 2);
		REQUIRE(menu[0] == menu_action_t::save);
		REQUIRE(menu[1] == menu_action_t::delete_file);
	}

	SECTION("base_dict loaded workspace not dirty")
	{
		file_entry_t entry;
		entry.type = file_type_t::base_dict;
		entry.is_workspace = true;
		const auto menu = derive_context_menu(entry, true, false);
		REQUIRE(menu.size() == 1);
		REQUIRE(menu[0] == menu_action_t::delete_file);
	}

	SECTION("user_dict not loaded workspace")
	{
		file_entry_t entry;
		entry.type = file_type_t::user_dict;
		entry.is_workspace = true;
		const auto menu = derive_context_menu(entry, false, false);
		REQUIRE(menu.size() == 1);
		REQUIRE(menu[0] == menu_action_t::delete_file);
	}

	SECTION("yaml_l10n any state")
	{
		file_entry_t entry;
		entry.type = file_type_t::yaml_l10n;
		entry.is_workspace = true;
		const auto menu = derive_context_menu(entry, true, true);
		REQUIRE(menu.size() == 1);
		REQUIRE(menu[0] == menu_action_t::delete_file);
	}
}

TEST_CASE("file_list_t, container operations", "[u]")
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
		first.language_tag = "EN";
		auto & second = list.add("C:/path/file.esp");
		REQUIRE(second.language_tag == "EN");
		REQUIRE(&first == &second);
	}
}

TEST_CASE("file_list_t, section grouping", "[u]")
{
	rc::prop(
	    "build_render_model groups entries by root_path",
	    []()
	{
		file_list_t list;
		const auto count = *rc::gen::inRange(1, 30);

		const auto root_paths = std::vector<std::string> { "C:/root_a", "C:/root_b", "C:/root_c" };

		struct expected_entry_t
		{
			std::string path;
			std::string display_text;
			std::string root_path;
			std::string subfolder;
		};

		std::vector<expected_entry_t> expected;

		for (int i = 0; i < count; ++i)
		{
			const auto ext =
			    *rc::gen::element(std::string(".esp"), std::string(".json"), std::string(".xml"), std::string(".yaml"));
			const auto root_idx = *rc::gen::inRange<size_t>(0, root_paths.size());
			const auto & root = root_paths[root_idx];
			const auto path = root + "/" + std::to_string(i) + "_file" + ext;

			auto & entry = list.add(path);
			entry.is_workspace = true;
			entry.root_path = root;

			const auto use_subfolder = *rc::gen::arbitrary<bool>();
			if (use_subfolder)
			{
				entry.workspace_subfolder =
				    *rc::gen::element(std::string("en"), std::string("pl"), std::string("de"), std::string("fr"));
			}

			expected_entry_t exp;
			exp.path = entry.path;
			exp.display_text = derive_display_name(entry, false, false);
			exp.root_path = root;
			exp.subfolder = entry.workspace_subfolder;
			expected.push_back(std::move(exp));
		}

		session_t session(codepage_t::windows_1252);
		const auto model = build_render_model(list, session, "");

		std::set<std::string> unique_roots;
		for (const auto & e : expected)
			unique_roots.insert(e.root_path);

		RC_ASSERT(model.roots.size() == unique_roots.size());

		for (const auto & root_node : model.roots)
		{
			RC_ASSERT(unique_roots.count(root_node.root_path) == 1);

			std::set<std::string> root_file_paths;
			for (const auto & item : root_node.items)
				root_file_paths.insert(item.path);

			std::set<std::string> expected_root_file_paths;
			for (const auto & e : expected)
			{
				if (e.root_path == root_node.root_path && e.subfolder.empty())
					expected_root_file_paths.insert(e.path);
			}
			RC_ASSERT(root_file_paths == expected_root_file_paths);

			std::set<std::string> expected_subfolders;
			for (const auto & e : expected)
			{
				if (e.root_path == root_node.root_path && !e.subfolder.empty())
					expected_subfolders.insert(e.subfolder);
			}

			RC_ASSERT(root_node.children.size() == expected_subfolders.size());

			for (const auto & child : root_node.children)
			{
				RC_ASSERT(expected_subfolders.count(child.label) == 1);

				std::set<std::string> child_paths;
				for (const auto & item : child.items)
					child_paths.insert(item.path);

				std::set<std::string> expected_child_paths;
				for (const auto & e : expected)
				{
					if (e.root_path == root_node.root_path && e.subfolder == child.label)
						expected_child_paths.insert(e.path);
				}
				RC_ASSERT(child_paths == expected_child_paths);
			}
		}
	});
}

TEST_CASE("file_list_t, workspace scan round-trip", "[i]")
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
	list.scan_roots({ temp_dir.string() });

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

TEST_CASE("file_list_t, scan finds nested files", "[i]")
{
	namespace fs = std::filesystem;

	const auto temp_dir = fs::temp_directory_path() / "yampt_test_depth";
	fs::remove_all(temp_dir);
	fs::create_directories(temp_dir / "en" / "deep");

	std::ofstream(temp_dir / "root.esp").put('x');
	std::ofstream(temp_dir / "en" / "level1.json").put('x');
	std::ofstream(temp_dir / "en" / "deep" / "level2.json").put('x');

	file_list_t list;
	list.scan_roots({ temp_dir.string() });

	REQUIRE(list.contains((temp_dir / "root.esp").string()));
	REQUIRE(list.contains((temp_dir / "en" / "level1.json").string()));
	REQUIRE(list.contains((temp_dir / "en" / "deep" / "level2.json").string()));

	fs::remove_all(temp_dir);
}

TEST_CASE("file_list_t, rescan after file deletion", "[i]")
{
	namespace fs = std::filesystem;

	const auto temp_dir = fs::temp_directory_path() / "yampt_test_rescan";
	fs::remove_all(temp_dir);
	fs::create_directories(temp_dir);

	std::ofstream(temp_dir / "keep.esp").put('x');
	std::ofstream(temp_dir / "remove.json").put('x');

	file_list_t list;
	list.scan_roots({ temp_dir.string() });
	REQUIRE(list.contains((temp_dir / "keep.esp").string()));
	REQUIRE(list.contains((temp_dir / "remove.json").string()));

	fs::remove(temp_dir / "remove.json");

	list.clear_workspace();
	list.scan_roots({ temp_dir.string() });
	REQUIRE(list.contains((temp_dir / "keep.esp").string()));
	REQUIRE_FALSE(list.contains((temp_dir / "remove.json").string()));

	fs::remove_all(temp_dir);
}
