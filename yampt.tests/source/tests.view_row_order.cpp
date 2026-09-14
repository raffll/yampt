#include <catch2/catch_all.hpp>
#include <decoder/sub_record_schema.hpp>
#include <model/view_row_order.hpp>
#include <utility/record_behavior.hpp>

namespace {

view_row_order::row_kind_input_t make_single(const std::string & type)
{
	view_row_order::row_kind_input_t input;
	input.type = type;
	input.size = 32;
	input.has_children = false;
	input.has_schema = false;
	return input;
}

view_row_order::row_kind_input_t make_block(const std::string & type)
{
	view_row_order::row_kind_input_t input;
	input.type = type;
	input.size = 32;
	input.has_children = true;
	input.has_schema = true;
	return input;
}

view_row_order::row_kind_input_t make_list(const std::string & type)
{
	view_row_order::row_kind_input_t input;
	input.type = type;
	input.size = 32;
	input.occurrence = 1;
	input.is_repeatable = true;
	return input;
}

view_row_order::row_kind_input_t make_group(const std::string & type)
{
	view_row_order::row_kind_input_t input;
	input.type = type;
	input.size = 0;
	input.has_children = true;
	return input;
}

view_row_order::row_kind_input_t make_placeholder(const std::string & type, bool has_schema)
{
	view_row_order::row_kind_input_t input;
	input.type = type;
	input.size = 0;
	input.has_children = false;
	input.is_optional_placeholder = true;
	input.has_schema = has_schema;
	return input;
}

view_row_order::row_kind_input_t make_header()
{
	view_row_order::row_kind_input_t input;
	input.type = "Record Header";
	return input;
}

} // namespace

TEST_CASE("view_row_order::structural_tier, classifies each row kind", "[u]")
{
	REQUIRE(view_row_order::structural_tier(make_header()) == view_row_order::tier_header);
	REQUIRE(view_row_order::structural_tier(make_single("NAME")) == view_row_order::tier_single_value);
	REQUIRE(view_row_order::structural_tier(make_block("NPDT")) == view_row_order::tier_data_block);
	REQUIRE(view_row_order::structural_tier(make_list("NPCO")) == view_row_order::tier_list);
	REQUIRE(view_row_order::structural_tier(make_group("ANAM")) == view_row_order::tier_list);
}

TEST_CASE("view_row_order::structural_tier, placeholder classified by schema not empty shape", "[u]")
{
	REQUIRE(view_row_order::structural_tier(make_placeholder("SCRI", false)) == view_row_order::tier_single_value);
	REQUIRE(view_row_order::structural_tier(make_placeholder("NPDT", true)) == view_row_order::tier_data_block);
}

TEST_CASE("view_row_order::rank, header sorts before everything", "[u]")
{
	const std::vector<std::string> roster = { "NAME", "NPDT" };
	REQUIRE(view_row_order::rank(make_header(), roster) < view_row_order::rank(make_single("NAME"), roster));
}

TEST_CASE("view_row_order::rank, single value before data block before list", "[u]")
{
	const std::vector<std::string> roster = { "NAME", "NPDT", "NPCO" };
	const int single = view_row_order::rank(make_single("NAME"), roster);
	const int block = view_row_order::rank(make_block("NPDT"), roster);
	const int list = view_row_order::rank(make_list("NPCO"), roster);

	REQUIRE(single < block);
	REQUIRE(block < list);
}

TEST_CASE("view_row_order::rank, roster order breaks ties within a tier", "[u]")
{
	const std::vector<std::string> roster = { "NAME", "FNAM", "MODL" };
	const int name = view_row_order::rank(make_single("NAME"), roster);
	const int fnam = view_row_order::rank(make_single("FNAM"), roster);
	const int modl = view_row_order::rank(make_single("MODL"), roster);

	REQUIRE(name < fnam);
	REQUIRE(fnam < modl);
}

TEST_CASE("view_row_order::rank, unlisted type ranks after listed within same tier", "[u]")
{
	const std::vector<std::string> roster = { "NAME", "FNAM" };
	const int listed = view_row_order::rank(make_single("FNAM"), roster);
	const int unlisted = view_row_order::rank(make_single("ZZZZ"), roster);

	REQUIRE(listed < unlisted);
}

TEST_CASE("view_row_order::rank, FACT RNAM sits after FADT and before reactions", "[u]")
{
	const std::vector<std::string> roster = { "NAME", "FNAM", "FADT", "RNAM", "ANAM", "INTV" };

	const int fadt = view_row_order::rank(make_block("FADT"), roster);
	const int rnam = view_row_order::rank(make_list("RNAM"), roster);
	const int reaction = view_row_order::rank(make_group("ANAM"), roster);

	REQUIRE(fadt < rnam);
	REQUIRE(rnam < reaction);
}

static int index_of(const std::vector<std::string> & order, const std::string & type)
{
	for (size_t position = 0; position < order.size(); ++position)
	{
		if (order[position] == type)
			return static_cast<int>(position);
	}

	return -1;
}

static std::vector<std::string> composition_order(const std::string & record_type)
{
	std::vector<std::string> order;
	for (const auto & entry : record_composition(record_type))
		order.push_back(entry.sub_type);

	return order;
}

TEST_CASE("record_composition, NPC_ orders NAME before MODL before FNAM", "[u]")
{
	const auto order = composition_order("NPC_");

	const int name = index_of(order, "NAME");
	const int modl = index_of(order, "MODL");
	const int fnam = index_of(order, "FNAM");

	REQUIRE(name >= 0);
	REQUIRE(name < modl);
	REQUIRE(modl < fnam);
}

static bool schema_has_sub_type(const std::string & sub_type)
{
	for (const auto & schema : all_schemas())
	{
		if (sub_type == schema.sub_type)
			return true;
	}

	return false;
}

TEST_CASE("record_composition, every composed sub-record type has a schema", "[u]")
{
	static const std::vector<std::string> record_types = {
		"ACTI", "ALCH", "APPA", "ARMO", "BODY", "BOOK", "BSGN", "CLAS", "CLOT", "CONT", "CREA",
		"DOOR", "ENCH", "GLOB", "GMST", "INGR", "LIGH", "LOCK", "MGEF", "MISC", "NPC_",
		"PROB", "RACE", "REPA", "SKIL", "SNDG", "SOUN", "SPEL", "SSCR", "STAT", "WEAP",
		"DIAL", "INFO", "FACT", "LEVI", "LEVC", "REGN", "SCPT", "PGRD", "LAND", "CELL", "LTEX"
	};

	for (const auto & record_type : record_types)
	{
		for (const auto & entry : record_composition(record_type))
		{
			INFO("composed sub-record without schema: " << record_type << " " << entry.sub_type);
			REQUIRE(schema_has_sub_type(entry.sub_type));
		}
	}
}

TEST_CASE("view_row_order::rank, WEAP composition places NAME before MODL before FNAM before SCRI", "[u]")
{
	const auto order = composition_order("WEAP");

	const int name = view_row_order::rank(make_single("NAME"), order);
	const int modl = view_row_order::rank(make_single("MODL"), order);
	const int fnam = view_row_order::rank(make_single("FNAM"), order);
	const int scri = view_row_order::rank(make_single("SCRI"), order);

	REQUIRE(name < modl);
	REQUIRE(modl < fnam);
	REQUIRE(fnam < scri);
}
