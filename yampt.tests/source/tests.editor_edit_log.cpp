#include <catch2/catch_all.hpp>
#include <model/edit_log.hpp>

TEST_CASE("edit_log_t::record_field_edit, appends a described entry", "[u]")
{
	edit_log_t log;
	log.record_field_edit({ "Morrowind.esm", "NPC_", "fargoth", "FNAM", "Fargoth PL" });

	REQUIRE(log.entries().size() == 1);
	REQUIRE(log.entries()[0].plugin_filename == "Morrowind.esm");
	REQUIRE(log.entries()[0].description == "edited NPC_:fargoth FNAM = Fargoth PL");
	REQUIRE(!log.entries()[0].timestamp.empty());
}

TEST_CASE("edit_log_t::record_field_edit, omits empty field name", "[u]")
{
	edit_log_t log;
	log.record_field_edit({ "Plugin.esp", "GMST", "sIntro", "", "Witaj" });

	REQUIRE(log.entries().size() == 1);
	REQUIRE(log.entries()[0].description == "edited GMST:sIntro = Witaj");
}

TEST_CASE("edit_log_t::record_record_removal, appends a removal entry", "[u]")
{
	edit_log_t log;
	log.record_record_removal({ "Mod.esp", "CELL", "Balmora" });

	REQUIRE(log.entries().size() == 1);
	REQUIRE(log.entries()[0].plugin_filename == "Mod.esp");
	REQUIRE(log.entries()[0].description == "removed CELL:Balmora");
}

TEST_CASE("edit_log_t::clear, removes all entries", "[u]")
{
	edit_log_t log;
	log.record_record_removal({ "Mod.esp", "CELL", "Balmora" });
	log.record_field_edit({ "Mod.esp", "NPC_", "guard", "FNAM", "Straznik" });

	REQUIRE(log.entries().size() == 2);

	log.clear();

	REQUIRE(log.entries().empty());
}

TEST_CASE("edit_log_t, preserves chronological append order", "[u]")
{
	edit_log_t log;
	log.record_field_edit({ "A.esp", "NPC_", "first", "FNAM", "one" });
	log.record_record_removal({ "A.esp", "CELL", "second" });

	REQUIRE(log.entries().size() == 2);
	REQUIRE(log.entries()[0].description == "edited NPC_:first FNAM = one");
	REQUIRE(log.entries()[1].description == "removed CELL:second");
}
