#include <catch2/catch_all.hpp>
#include <io/eet_converter.hpp>
#include <rapidcheck/catch.h>
#include <utility/app_logger.hpp>
#include <rapidcheck.h>

namespace {

struct valid_type_combo_t
{
	std::string rec_type;
	std::string sub_type;
};

rc::Gen<valid_type_combo_t> gen_valid_type_combo()
{
	return rc::gen::element(
	    valid_type_combo_t { "NPC_", "FNAM" },
	    valid_type_combo_t { "SPEL", "FNAM" },
	    valid_type_combo_t { "ARMO", "FNAM" },
	    valid_type_combo_t { "BOOK", "FNAM" },
	    valid_type_combo_t { "CONT", "FNAM" },
	    valid_type_combo_t { "MISC", "FNAM" },
	    valid_type_combo_t { "CLOT", "FNAM" },
	    valid_type_combo_t { "CREA", "FNAM" },
	    valid_type_combo_t { "ALCH", "FNAM" },
	    valid_type_combo_t { "DOOR", "FNAM" },
	    valid_type_combo_t { "ACTI", "FNAM" },
	    valid_type_combo_t { "LIGH", "FNAM" },
	    valid_type_combo_t { "INGR", "FNAM" },
	    valid_type_combo_t { "CLAS", "FNAM" },
	    valid_type_combo_t { "FACT", "FNAM" },
	    valid_type_combo_t { "APPA", "FNAM" },
	    valid_type_combo_t { "REPA", "FNAM" },
	    valid_type_combo_t { "WEAP", "FNAM" },
	    valid_type_combo_t { "CELL", "NAME" },
	    valid_type_combo_t { "REGN", "NAME" },
	    valid_type_combo_t { "PGRD", "NAME" },
	    valid_type_combo_t { "CELL", "FNAM" },
	    valid_type_combo_t { "REGN", "FNAM" },
	    valid_type_combo_t { "CELL", "DNAM" },
	    valid_type_combo_t { "NPC_", "DNAM" },
	    valid_type_combo_t { "NPC_", "CNDT" },
	    valid_type_combo_t { "BOOK", "TEXT" },
	    valid_type_combo_t { "SCPT", "MSGB" },
	    valid_type_combo_t { "SCPT", "CELL" },
	    valid_type_combo_t { "SCPT", "SAY_" },
	    valid_type_combo_t { "SCPT", "DIAL" },
	    valid_type_combo_t { "MGEF", "DESC" },
	    valid_type_combo_t { "CLAS", "DESC" },
	    valid_type_combo_t { "FACT", "RNAM" },
	    valid_type_combo_t { "GMST", "STRV" },
	    valid_type_combo_t { "DIAL", "NAME" });
}

rc::Gen<std::string> gen_non_empty_text()
{
	return rc::gen::map(
	    rc::gen::container<std::string>(rc::gen::inRange('a', 'z')),
	    [](std::string value)
	{
		if (value.empty())
			value = "a";

		return value;
	});
}

} // namespace

TEST_CASE("eet_converter_t::import, round-trip property", "[u]")
{
	rc::prop(
	    "old_text == orig and new_text == trans for all valid entries",
	    []()
	{
		const auto combo = *gen_valid_type_combo();
		const auto orig = *gen_non_empty_text();
		const auto trans = *gen_non_empty_text();
		const auto key_text = *gen_non_empty_text();

		eet_reader_t::eet_entry_t entry;
		entry.rec_type = combo.rec_type;
		entry.sub_type = combo.sub_type;
		entry.context = "";
		entry.key_text = key_text;
		entry.orig = orig;
		entry.trans = trans;
		entry.status_byte = 0x63;

		std::vector<eet_reader_t::eet_entry_t> entries = { entry };
		eet_converter_t converter(entries, codepage_t::windows_1252);

		const auto & dict = converter.get_dict();

		bool found = false;
		for (const auto & [type, chapter] : dict)
		{
			for (const auto & record : chapter.records)
			{
				if (record.old_text == orig && record.new_text == trans)
				{
					found = true;
					break;
				}
			}

			if (found)
				break;
		}

		RC_ASSERT(found);
	});
}

TEST_CASE("eet_converter_t::map_type, exhaustive type mapping", "[u]")
{
	struct mapping_t
	{
		std::string rec_type;
		std::string sub_type;
		rec_type_t expected;
	};

	const std::vector<mapping_t> known_mappings = {
		{ "NPC_", "FNAM", rec_type_t::fnam }, { "SPEL", "FNAM", rec_type_t::fnam },
		{ "ARMO", "FNAM", rec_type_t::fnam }, { "BOOK", "FNAM", rec_type_t::fnam },
		{ "CONT", "FNAM", rec_type_t::fnam }, { "MISC", "FNAM", rec_type_t::fnam },
		{ "CLOT", "FNAM", rec_type_t::fnam }, { "CREA", "FNAM", rec_type_t::fnam },
		{ "ALCH", "FNAM", rec_type_t::fnam }, { "DOOR", "FNAM", rec_type_t::fnam },
		{ "ACTI", "FNAM", rec_type_t::fnam }, { "LIGH", "FNAM", rec_type_t::fnam },
		{ "INGR", "FNAM", rec_type_t::fnam }, { "CLAS", "FNAM", rec_type_t::fnam },
		{ "FACT", "FNAM", rec_type_t::fnam }, { "APPA", "FNAM", rec_type_t::fnam },
		{ "REPA", "FNAM", rec_type_t::fnam }, { "WEAP", "FNAM", rec_type_t::fnam },
		{ "CELL", "NAME", rec_type_t::cell },
		{ "REGN", "NAME", rec_type_t::cell }, { "PGRD", "NAME", rec_type_t::cell },
		{ "CELL", "FNAM", rec_type_t::cell }, { "REGN", "FNAM", rec_type_t::cell },
		{ "CELL", "DNAM", rec_type_t::cell }, { "NPC_", "DNAM", rec_type_t::fnam },
		{ "NPC_", "CNDT", rec_type_t::fnam }, { "BOOK", "TEXT", rec_type_t::text },
		{ "SCPT", "MSGB", rec_type_t::sctx }, { "SCPT", "CELL", rec_type_t::sctx },
		{ "SCPT", "SAY_", rec_type_t::sctx }, { "SCPT", "DIAL", rec_type_t::sctx },
		{ "MGEF", "DESC", rec_type_t::desc }, { "CLAS", "DESC", rec_type_t::desc },
		{ "FACT", "RNAM", rec_type_t::rnam }, { "GMST", "STRV", rec_type_t::gmst },
		{ "DIAL", "NAME", rec_type_t::dial },
	};

	REQUIRE(known_mappings.size() == 36);

	for (const auto & mapping : known_mappings)
	{
		eet_reader_t::eet_entry_t entry;
		entry.rec_type = mapping.rec_type;
		entry.sub_type = mapping.sub_type;
		entry.context = "";
		entry.key_text = "test_key";
		entry.orig = "original";
		entry.trans = "translated";
		entry.status_byte = 0x63;

		std::vector<eet_reader_t::eet_entry_t> entries = { entry };
		eet_converter_t converter(entries, codepage_t::windows_1252);

		REQUIRE(converter.converted_count() == 1);
		REQUIRE(converter.skipped_count() == 0);

		const auto & dict = converter.get_dict();
		bool found = false;
		for (const auto & [type, chapter] : dict)
		{
			if (type != mapping.expected)
				continue;

			for (const auto & record : chapter.records)
			{
				if (record.old_text == "original" && record.new_text == "translated")
				{
					found = true;
					break;
				}
			}

			if (found)
				break;
		}

		INFO("rec_type=" << mapping.rec_type << " sub_type=" << mapping.sub_type);
		REQUIRE(found);
	}
}

TEST_CASE("eet_converter_t::map_type, unknown combinations", "[u]")
{
	struct unknown_combo_t
	{
		std::string rec_type;
		std::string sub_type;
	};

	const std::vector<unknown_combo_t> unknown_combos = {
		{ "NPC_", "NAME" }, { "SCPT", "FNAM" }, { "SCPT", "SCTX" },
		{ "BOOK", "DESC" }, { "INFO", "NAME" }, { "LEVI", "FNAM" }, { "GLOB", "FNAM" },
	};

	for (const auto & combo : unknown_combos)
	{
		eet_reader_t::eet_entry_t entry;
		entry.rec_type = combo.rec_type;
		entry.sub_type = combo.sub_type;
		entry.context = "";
		entry.key_text = "test_key";
		entry.orig = "original";
		entry.trans = "translated";
		entry.status_byte = 0x63;

		std::vector<eet_reader_t::eet_entry_t> entries = { entry };
		eet_converter_t converter(entries, codepage_t::windows_1252);

		INFO("rec_type=" << combo.rec_type << " sub_type=" << combo.sub_type);
		REQUIRE(converter.converted_count() == 0);
		REQUIRE(converter.skipped_count() == 1);
	}
}

TEST_CASE("eet_converter_t::map_type, WEAP FNAM maps to fnam with bare key", "[u]")
{
	eet_reader_t::eet_entry_t entry;
	entry.rec_type = "WEAP";
	entry.sub_type = "FNAM";
	entry.context = "";
	entry.key_text = "steel dagger";
	entry.orig = "Steel Dagger";
	entry.trans = "Stahldolch";
	entry.status_byte = 0x63;

	std::vector<eet_reader_t::eet_entry_t> entries = { entry };
	eet_converter_t converter(entries, codepage_t::windows_1252);

	REQUIRE(converter.converted_count() == 1);
	REQUIRE(converter.skipped_count() == 0);

	const auto & dict = converter.get_dict();
	const auto it_chapter = dict.find(rec_type_t::fnam);
	REQUIRE(it_chapter != dict.end());

	const auto * record = it_chapter->second.find("steel dagger");
	REQUIRE(record != nullptr);
	REQUIRE(record->old_text == "Steel Dagger");
	REQUIRE(record->new_text == "Stahldolch");
}

TEST_CASE("eet_converter_t::build_key_text, SCPT messages key script_name^message in sctx", "[u]")
{
	const std::vector<std::string> sub_types = { "MSGB", "SAY_", "CELL", "DIAL" };

	for (const auto & sub_type : sub_types)
	{
		eet_reader_t::eet_entry_t entry;
		entry.rec_type = "SCPT";
		entry.sub_type = sub_type;
		entry.context = "";
		entry.key_text = "SomeScript";
		entry.orig = "Hello";
		entry.trans = "Hallo";
		entry.status_byte = 0x63;

		std::vector<eet_reader_t::eet_entry_t> entries = { entry };
		eet_converter_t converter(entries, codepage_t::windows_1252);

		INFO("sub_type=" << sub_type);
		REQUIRE(converter.converted_count() == 1);
		REQUIRE(converter.skipped_count() == 0);

		const auto & dict = converter.get_dict();
		const auto it_sctx = dict.find(rec_type_t::sctx);
		REQUIRE(it_sctx != dict.end());

		const auto * record = it_sctx->second.find("SomeScript^Hello");
		REQUIRE(record != nullptr);
		REQUIRE(record->old_text == "Hello");
		REQUIRE(record->new_text == "Hallo");

		const auto it_bnam = dict.find(rec_type_t::bnam);
		REQUIRE(it_bnam != dict.end());
		REQUIRE(it_bnam->second.empty());
	}
}

TEST_CASE("eet_converter_t::map_status, 0x63 translated and 0xFF untranslated", "[u]")
{
	eet_reader_t::eet_entry_t translated_entry;
	translated_entry.rec_type = "WEAP";
	translated_entry.sub_type = "FNAM";
	translated_entry.context = "";
	translated_entry.key_text = "steel dagger";
	translated_entry.orig = "Steel Dagger";
	translated_entry.trans = "Stahldolch";
	translated_entry.status_byte = 0x63;

	eet_reader_t::eet_entry_t untranslated_entry;
	untranslated_entry.rec_type = "WEAP";
	untranslated_entry.sub_type = "FNAM";
	untranslated_entry.context = "";
	untranslated_entry.key_text = "iron spear";
	untranslated_entry.orig = "Iron Spear";
	untranslated_entry.trans = "Iron Spear";
	untranslated_entry.status_byte = 0xFF;

	std::vector<eet_reader_t::eet_entry_t> entries = { translated_entry, untranslated_entry };
	eet_converter_t converter(entries, codepage_t::windows_1252);

	REQUIRE(converter.converted_count() == 2);

	const auto & dict = converter.get_dict();
	const auto it_chapter = dict.find(rec_type_t::fnam);
	REQUIRE(it_chapter != dict.end());

	const auto * translated_record = it_chapter->second.find("steel dagger");
	REQUIRE(translated_record != nullptr);
	REQUIRE(translated_record->status == status_t::translated);

	const auto * untranslated_record = it_chapter->second.find("iron spear");
	REQUIRE(untranslated_record != nullptr);
	REQUIRE(untranslated_record->status == status_t::untranslated);
}
