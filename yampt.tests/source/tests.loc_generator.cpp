#include <catch2/catch_all.hpp>
#include <creator/loc_generator.hpp>

TEST_CASE("loc_generator::derive_esm_name, preserves original case", "[u]")
{
	REQUIRE(loc_generator::derive_esm_name("Morrowind_en_pl.json") == "Morrowind");
	REQUIRE(loc_generator::derive_esm_name("Tribunal_EN_PL.json") == "Tribunal");
	REQUIRE(loc_generator::derive_esm_name("Bloodmoon.esm") == "Bloodmoon");
}

TEST_CASE("loc_generator::derive_esm_name, strips language suffix", "[u]")
{
	REQUIRE(loc_generator::derive_esm_name("Morrowind_en_de.json") == "Morrowind");
	REQUIRE(loc_generator::derive_esm_name("Some_Mod_fr_it.xml") == "Some_Mod");
	REQUIRE(loc_generator::derive_esm_name("NoSuffix.json") == "NoSuffix");
}

TEST_CASE("loc_generator::derive_esm_name, handles path and extension", "[u]")
{
	REQUIRE(loc_generator::derive_esm_name("C:/data/Morrowind_en_pl.json") == "Morrowind");
	REQUIRE(loc_generator::derive_esm_name("C:\\mods\\Great_House_ru_ru.json") == "Great_House");
}
