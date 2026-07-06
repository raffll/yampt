#include <catch2/catch_all.hpp>
#include <view/display_name.hpp>

TEST_CASE("display_name_t::to_string, filename only", "[u]")
{
	display_name_t display("Morrowind.json");
	REQUIRE(display.to_string() == "Morrowind.json");
}

TEST_CASE("display_name_t::to_string, dirty prefix", "[u]")
{
	display_name_t display("test.json");
	display.set_dirty(true);
	REQUIRE(display.to_string() == "* test.json");
}

TEST_CASE("display_name_t::to_string, base tag", "[u]")
{
	display_name_t display("base.json");
	display.set_kind(dict_kind_t::base);
	REQUIRE(display.to_string() == "[BASE] base.json");
}

TEST_CASE("display_name_t::to_string, language tag", "[u]")
{
	display_name_t display("plugin.esm");
	display.set_language("PL");
	REQUIRE(display.to_string() == "[PL] plugin.esm");
}

TEST_CASE("display_name_t::to_string, unloaded tag", "[u]")
{
	display_name_t display("dict.json");
	display.set_unloaded(true);
	REQUIRE(display.to_string() == "[UNLOADED] dict.json");
}

TEST_CASE("display_name_t::to_string, all flags combined", "[u]")
{
	display_name_t display("file.json");
	display.set_dirty(true);
	display.set_unloaded(true);
	display.set_kind(dict_kind_t::base);
	display.set_wip(true);
	display.set_language("DE");

	REQUIRE(display.to_string() == "* [UNLOADED] [BASE] [WIP] [DE] file.json");
}

TEST_CASE("display_name_t::to_string, wip without base", "[u]")
{
	display_name_t display("wip.json");
	display.set_wip(true);
	REQUIRE(display.to_string() == "[WIP] wip.json");
}

TEST_CASE("display_name_t::filename, returns stored filename", "[u]")
{
	display_name_t display("original.json");
	REQUIRE(display.filename() == "original.json");

	display.set_filename("changed.json");
	REQUIRE(display.filename() == "changed.json");
}
