#include <catch2/catch_all.hpp>
#include <io/folder_plugin_order.hpp>
#include <set>
#include <string>
#include <vector>

namespace {

file_exists_fn_t make_exists(const std::set<std::string> & existing_paths)
{
	return [existing_paths](const std::string & path) { return existing_paths.count(path) > 0; };
}

} // namespace

TEST_CASE("folder_plugin_order::order_folder_plugins, masters first keeping folder order", "[u]")
{
	const std::vector<std::string> paths = { "D/b.esp", "D/Master.ESM", "D/a.esp", "D/other.esm" };

	const auto ordered = folder_plugin_order::order_folder_plugins({ paths, make_exists({}) });

	REQUIRE(ordered == std::vector<std::string> { "D/Master.ESM", "D/other.esm", "D/b.esp", "D/a.esp" });
}

TEST_CASE("folder_plugin_order::order_folder_plugins, openmw stub is followed by its real file", "[u]")
{
	const std::vector<std::string> paths = { "D/a.esp", "D/Mod.omwaddon.esp", "D/b.esp" };

	const auto ordered = folder_plugin_order::order_folder_plugins({ paths, make_exists({ "D/Mod.omwaddon" }) });

	REQUIRE(ordered == std::vector<std::string> { "D/a.esp", "D/Mod.omwaddon.esp", "D/Mod.omwaddon", "D/b.esp" });
}

TEST_CASE("folder_plugin_order::order_folder_plugins, omwscripts stub resolves the same way", "[u]")
{
	const std::vector<std::string> paths = { "D/Lua.omwscript.esp" };

	const auto ordered = folder_plugin_order::order_folder_plugins({ paths, make_exists({ "D/Lua.omwscript" }) });

	REQUIRE(ordered == std::vector<std::string> { "D/Lua.omwscript.esp", "D/Lua.omwscript" });
}

TEST_CASE("folder_plugin_order::order_folder_plugins, stub without its real file loads alone", "[u]")
{
	const std::vector<std::string> paths = { "D/Mod.omwaddon.esp" };

	const auto ordered = folder_plugin_order::order_folder_plugins({ paths, make_exists({}) });

	REQUIRE(ordered == std::vector<std::string> { "D/Mod.omwaddon.esp" });
}
