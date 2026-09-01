#include <catch2/catch_all.hpp>
#include <model/edit_history.hpp>

TEST_CASE("edit_history_t::record_field_edit, appends a described entry", "[u]")
{
	edit_history_t history;
	history.record_field_edit({ "Morrowind.esm", "NPC_", "fargoth", "FNAM", "Fargoth PL" });

	REQUIRE(history.entries().size() == 1);
	REQUIRE(history.entries()[0].plugin_filename == "Morrowind.esm");
	REQUIRE(history.entries()[0].description == "edited NPC_:fargoth FNAM = Fargoth PL");
	REQUIRE(!history.entries()[0].timestamp.empty());
}

TEST_CASE("edit_history_t::record_field_edit, omits empty field name", "[u]")
{
	edit_history_t history;
	history.record_field_edit({ "Plugin.esp", "GMST", "sIntro", "", "Witaj" });

	REQUIRE(history.entries().size() == 1);
	REQUIRE(history.entries()[0].description == "edited GMST:sIntro = Witaj");
}

TEST_CASE("edit_history_t::record_record_removal, appends a removal entry", "[u]")
{
	edit_history_t history;
	history.record_record_removal({ "Mod.esp", "CELL", "Balmora" });

	REQUIRE(history.entries().size() == 1);
	REQUIRE(history.entries()[0].plugin_filename == "Mod.esp");
	REQUIRE(history.entries()[0].description == "removed CELL:Balmora");
}

TEST_CASE("edit_history_t::clear, removes all entries", "[u]")
{
	edit_history_t history;
	history.record_record_removal({ "Mod.esp", "CELL", "Balmora" });
	history.record_field_edit({ "Mod.esp", "NPC_", "guard", "FNAM", "Straznik" });

	REQUIRE(history.entries().size() == 2);

	history.clear();

	REQUIRE(history.entries().empty());
}

TEST_CASE("edit_history_t, preserves chronological append order", "[u]")
{
	edit_history_t history;
	history.record_field_edit({ "A.esp", "NPC_", "first", "FNAM", "one" });
	history.record_record_removal({ "A.esp", "CELL", "second" });

	REQUIRE(history.entries().size() == 2);
	REQUIRE(history.entries()[0].description == "edited NPC_:first FNAM = one");
	REQUIRE(history.entries()[1].description == "removed CELL:second");
}
