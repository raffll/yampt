#include <catch2/catch_all.hpp>
#include <io/eet_converter.hpp>
#include <rapidcheck/catch.h>
#include <utility/tools.hpp>
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
	    valid_type_combo_t { "CELL", "NAME" },
	    valid_type_combo_t { "REGN", "NAME" },
	    valid_type_combo_t { "PGRD", "NAME" },
	    valid_type_combo_t { "CELL", "FNAM" },
	    valid_type_combo_t { "REGN", "FNAM" },
	    valid_type_combo_t { "CELL", "DNAM" },
	    valid_type_combo_t { "NPC_", "DNAM" },
	    valid_type_combo_t { "NPC_", "CNDT" },
	    valid_type_combo_t { "BOOK", "TEXT" },
	    valid_type_combo_t { "SCPT", "SCTX" },
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
	return rc::gen::suchThat(rc::gen::string<std::string>(), [](const std::string & value) { return !value.empty(); });
}

} // namespace

TEST_CASE("eet_converter_t, import round-trip property", "[u]")
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
		eet_converter_t converter(entries);

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
		tools_t::rec_type_t expected;
	};

	const std::vector<mapping_t> known_mappings = {
		{ "NPC_", "FNAM", tools_t::rec_type_t::fnam }, { "SPEL", "FNAM", tools_t::rec_type_t::fnam },
		{ "ARMO", "FNAM", tools_t::rec_type_t::fnam }, { "BOOK", "FNAM", tools_t::rec_type_t::fnam },
		{ "CONT", "FNAM", tools_t::rec_type_t::fnam }, { "MISC", "FNAM", tools_t::rec_type_t::fnam },
		{ "CLOT", "FNAM", tools_t::rec_type_t::fnam }, { "CREA", "FNAM", tools_t::rec_type_t::fnam },
		{ "ALCH", "FNAM", tools_t::rec_type_t::fnam }, { "DOOR", "FNAM", tools_t::rec_type_t::fnam },
		{ "ACTI", "FNAM", tools_t::rec_type_t::fnam }, { "LIGH", "FNAM", tools_t::rec_type_t::fnam },
		{ "INGR", "FNAM", tools_t::rec_type_t::fnam }, { "CLAS", "FNAM", tools_t::rec_type_t::fnam },
		{ "FACT", "FNAM", tools_t::rec_type_t::fnam }, { "APPA", "FNAM", tools_t::rec_type_t::fnam },
		{ "REPA", "FNAM", tools_t::rec_type_t::fnam }, { "CELL", "NAME", tools_t::rec_type_t::cell },
		{ "REGN", "NAME", tools_t::rec_type_t::cell }, { "PGRD", "NAME", tools_t::rec_type_t::cell },
		{ "CELL", "FNAM", tools_t::rec_type_t::cell }, { "REGN", "FNAM", tools_t::rec_type_t::cell },
		{ "CELL", "DNAM", tools_t::rec_type_t::cell }, { "NPC_", "DNAM", tools_t::rec_type_t::fnam },
		{ "NPC_", "CNDT", tools_t::rec_type_t::fnam }, { "BOOK", "TEXT", tools_t::rec_type_t::text },
		{ "SCPT", "SCTX", tools_t::rec_type_t::sctx }, { "SCPT", "MSGB", tools_t::rec_type_t::bnam },
		{ "SCPT", "CELL", tools_t::rec_type_t::bnam }, { "SCPT", "SAY_", tools_t::rec_type_t::bnam },
		{ "SCPT", "DIAL", tools_t::rec_type_t::bnam }, { "MGEF", "DESC", tools_t::rec_type_t::desc },
		{ "CLAS", "DESC", tools_t::rec_type_t::desc }, { "FACT", "RNAM", tools_t::rec_type_t::rnam },
		{ "GMST", "STRV", tools_t::rec_type_t::gmst }, { "DIAL", "NAME", tools_t::rec_type_t::dial },
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
		eet_converter_t converter(entries);

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
		{ "WEAP", "FNAM" }, { "NPC_", "NAME" }, { "SCPT", "FNAM" }, { "BOOK", "DESC" },
		{ "INFO", "NAME" }, { "LEVI", "FNAM" }, { "GLOB", "FNAM" },
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
		eet_converter_t converter(entries);

		INFO("rec_type=" << combo.rec_type << " sub_type=" << combo.sub_type);
		REQUIRE(converter.converted_count() == 0);
		REQUIRE(converter.skipped_count() == 1);
	}
}
