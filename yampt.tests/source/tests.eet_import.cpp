#include <catch2/catch_all.hpp>
#include <creator/eet_dict_builder.hpp>
#include <io/eet_filename.hpp>
#include <utility/domain_types.hpp>

namespace {

record_entry_t make_entry(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    status_t status)
{
	record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = status;
	return entry;
}

} // namespace

TEST_CASE("eet_filename::parse, name and hash split on last underscore", "[eet_import]")
{
	const auto parts = eet_filename::parse("Morrowind Rebirth [Main]_4C483EEC.eet");
	REQUIRE(parts.has_value());
	REQUIRE(parts->name == "Morrowind Rebirth [Main]");
	REQUIRE(parts->hash == "4C483EEC");
}

TEST_CASE("eet_filename::parse, name containing underscores keeps all but the last segment", "[eet_import]")
{
	const auto parts = eet_filename::parse("My_Mod_ABCD1234.eet");
	REQUIRE(parts.has_value());
	REQUIRE(parts->name == "My_Mod");
	REQUIRE(parts->hash == "ABCD1234");
}

TEST_CASE("eet_filename::parse, strips a full path and is case-insensitive on the extension", "[eet_import]")
{
	const auto parts = eet_filename::parse("C:/data/Plugin Name_DEADBEEF.EET");
	REQUIRE(parts.has_value());
	REQUIRE(parts->name == "Plugin Name");
	REQUIRE(parts->hash == "DEADBEEF");
}

TEST_CASE("eet_filename::parse, no underscore returns nullopt", "[eet_import]")
{
	REQUIRE_FALSE(eet_filename::parse("NoHashHere.eet").has_value());
}

TEST_CASE("eet_filename::parse, trailing underscore / empty hash returns nullopt", "[eet_import]")
{
	REQUIRE_FALSE(eet_filename::parse("Plugin_.eet").has_value());
}

TEST_CASE("eet_filename::parse, empty name returns nullopt", "[eet_import]")
{
	REQUIRE_FALSE(eet_filename::parse("_ABCD1234.eet").has_value());
}

TEST_CASE("eet_dict_builder_t::build, INFO entry filled by old_text match when unambiguous", "[eet_import]")
{
	auto native = domain_types::initialize_dict();
	native.at(rec_type_t::info).insert(make_entry("T^Topic^InfoId", "Hello.", "Hello.", status_t::untranslated));

	auto pool = domain_types::initialize_dict();
	pool.at(rec_type_t::info).insert(make_entry("12345", "Hello.", "Witaj.", status_t::translated));

	const auto result = eet_dict_builder_t::build(native, pool);

	const auto * record = result.at(rec_type_t::info).find("T^Topic^InfoId");
	REQUIRE(record != nullptr);
	REQUIRE(record->new_text == "Witaj.");
	REQUIRE(record->status == status_t::translated);
}

TEST_CASE("eet_dict_builder_t::build, INFO duplicate orig under two contexts yields ambiguous", "[eet_import]")
{
	auto native = domain_types::initialize_dict();
	native.at(rec_type_t::info).insert(make_entry("T^Topic^InfoId", "Hello.", "Hello.", status_t::untranslated));

	auto pool = domain_types::initialize_dict();
	pool.at(rec_type_t::info).insert(make_entry("12345", "Hello.", "Witaj.", status_t::translated));
	pool.at(rec_type_t::info).insert(make_entry("67890", "Hello.", "Czesc.", status_t::translated));

	const auto result = eet_dict_builder_t::build(native, pool);

	const auto * record = result.at(rec_type_t::info).find("T^Topic^InfoId");
	REQUIRE(record != nullptr);
	REQUIRE(record->status == status_t::ambiguous);
}

TEST_CASE("eet_dict_builder_t::build, no pool match leaves native entry untranslated and unchanged", "[eet_import]")
{
	auto native = domain_types::initialize_dict();
	native.at(rec_type_t::info).insert(make_entry("T^Topic^InfoId", "Hello.", "Hello.", status_t::untranslated));

	auto pool = domain_types::initialize_dict();
	pool.at(rec_type_t::info).insert(make_entry("12345", "Goodbye.", "Zegnaj.", status_t::translated));

	const auto result = eet_dict_builder_t::build(native, pool);

	const auto * record = result.at(rec_type_t::info).find("T^Topic^InfoId");
	REQUIRE(record != nullptr);
	REQUIRE(record->new_text == "Hello.");
	REQUIRE(record->status == status_t::untranslated);
}

TEST_CASE("eet_dict_builder_t::build, BNAM per-message filled by old_text match", "[eet_import]")
{
	auto native = domain_types::initialize_dict();
	native.at(rec_type_t::bnam)
	    .insert(make_entry("T^Topic^InfoId^Sell.", "Sell.", "Sell.", status_t::untranslated));

	auto pool = domain_types::initialize_dict();
	pool.at(rec_type_t::bnam).insert(make_entry("Sell.", "Sell.", "Sprzedaj.", status_t::translated));

	const auto result = eet_dict_builder_t::build(native, pool);

	const auto * record = result.at(rec_type_t::bnam).find("T^Topic^InfoId^Sell.");
	REQUIRE(record != nullptr);
	REQUIRE(record->new_text == "Sprzedaj.");
	REQUIRE(record->status == status_t::translated);
}

TEST_CASE("eet_dict_builder_t::build, FNAM filled by exact id match", "[eet_import]")
{
	auto native = domain_types::initialize_dict();
	native.at(rec_type_t::fnam)
	    .insert(make_entry("steel dagger", "Steel Dagger", "Steel Dagger", status_t::untranslated));

	auto pool = domain_types::initialize_dict();
	pool.at(rec_type_t::fnam).insert(make_entry("steel dagger", "Steel Dagger", "Stahldolch", status_t::translated));

	const auto result = eet_dict_builder_t::build(native, pool);

	const auto * record = result.at(rec_type_t::fnam).find("steel dagger");
	REQUIRE(record != nullptr);
	REQUIRE(record->new_text == "Stahldolch");
	REQUIRE(record->status == status_t::translated);
}

TEST_CASE("eet_dict_builder_t::build, ANAM/SCVR place value absent from native CELL is inserted", "[eet_import]")
{
	auto native = domain_types::initialize_dict();

	auto pool = domain_types::initialize_dict();
	pool.at(rec_type_t::cell).insert(make_entry("Balmora", "Balmora", "Balmora PL", status_t::translated));

	const auto result = eet_dict_builder_t::build(native, pool);

	const auto * record = result.at(rec_type_t::cell).find("Balmora");
	REQUIRE(record != nullptr);
	REQUIRE(record->new_text == "Balmora PL");
	REQUIRE(record->status == status_t::translated);
}

TEST_CASE("eet_dict_builder_t::build, existing native CELL entry is filled not duplicated", "[eet_import]")
{
	auto native = domain_types::initialize_dict();
	native.at(rec_type_t::cell).insert(make_entry("Balmora", "Balmora", "Balmora", status_t::untranslated));

	auto pool = domain_types::initialize_dict();
	pool.at(rec_type_t::cell).insert(make_entry("Balmora", "Balmora", "Balmora PL", status_t::translated));

	const auto result = eet_dict_builder_t::build(native, pool);

	REQUIRE(result.at(rec_type_t::cell).size() == 1);
	const auto * record = result.at(rec_type_t::cell).find("Balmora");
	REQUIRE(record != nullptr);
	REQUIRE(record->new_text == "Balmora PL");
	REQUIRE(record->status == status_t::translated);
}
