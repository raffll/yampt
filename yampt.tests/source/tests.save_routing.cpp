#include <catch2/catch_all.hpp>
#include <controller/save_routing.hpp>

TEST_CASE("save_routing::decide_save_path, merged patch is created", "[u]")
{
	const std::set<std::string> created;
	REQUIRE(save_routing::decide_save_path("Merged Patch.esp", created) == save_routing::save_path_t::created_plugin);
}

TEST_CASE("save_routing::decide_save_path, name in created set is created", "[u]")
{
	const std::set<std::string> created = { "My Plugin.esp" };
	REQUIRE(save_routing::decide_save_path("My Plugin.esp", created) == save_routing::save_path_t::created_plugin);
}

TEST_CASE("save_routing::decide_save_path, other name is loaded", "[u]")
{
	const std::set<std::string> created = { "My Plugin.esp" };
	REQUIRE(save_routing::decide_save_path("Other.esp", created) == save_routing::save_path_t::loaded_plugin);
}

TEST_CASE("save_routing::decide_save_path, comparison ignores case", "[u]")
{
	const std::set<std::string> created = { "My Plugin.esp" };
	REQUIRE(save_routing::decide_save_path("my plugin.ESP", created) == save_routing::save_path_t::created_plugin);
	REQUIRE(save_routing::decide_save_path("merged patch.esp", created) == save_routing::save_path_t::created_plugin);
}

TEST_CASE("save_routing::select_copy_source_indices, picks only copy sources", "[u]")
{
	const std::vector<std::string> plugins = { "Morrowind.esm", "Tribunal.esm", "Active.esp", "Bloodmoon.esm" };
	const std::set<std::string> sources = { "Bloodmoon.esm", "Morrowind.esm" };

	const auto result = save_routing::select_copy_source_indices({ plugins, sources, 2 });

	REQUIRE(result == std::vector<int> { 0, 3 });
}

TEST_CASE("save_routing::select_copy_source_indices, excludes active plugin", "[u]")
{
	const std::vector<std::string> plugins = { "Morrowind.esm", "Active.esp" };
	const std::set<std::string> sources = { "Active.esp", "Morrowind.esm" };

	const auto result = save_routing::select_copy_source_indices({ plugins, sources, 1 });

	REQUIRE(result == std::vector<int> { 0 });
}

TEST_CASE("save_routing::select_copy_source_indices, de-duplicates repeated names", "[u]")
{
	const std::vector<std::string> plugins = { "Morrowind.esm", "morrowind.ESM", "Active.esp" };
	const std::set<std::string> sources = { "Morrowind.esm", "MORROWIND.esm" };

	const auto result = save_routing::select_copy_source_indices({ plugins, sources, 2 });

	REQUIRE(result == std::vector<int> { 0 });
}

TEST_CASE("save_routing::select_copy_source_indices, empty sources yields nothing", "[u]")
{
	const std::vector<std::string> plugins = { "Morrowind.esm", "Active.esp" };
	const std::set<std::string> sources;

	REQUIRE(save_routing::select_copy_source_indices({ plugins, sources, 1 }).empty());
}
