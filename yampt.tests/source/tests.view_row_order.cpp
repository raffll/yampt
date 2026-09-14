#include <catch2/catch_all.hpp>
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

TEST_CASE("sub_record_sort_order, begins with NAME FNAM MODL", "[u]")
{
	const auto & order = sub_record_sort_order();

	REQUIRE(order.size() >= 3);
	REQUIRE(order[0] == "NAME");
	REQUIRE(order[1] == "FNAM");
	REQUIRE(order[2] == "MODL");
}

TEST_CASE("view_row_order::rank, shared field keeps identical rank across record types", "[u]")
{
	const auto & order = sub_record_sort_order();

	const int fnam_first = view_row_order::rank(make_single("FNAM"), order);
	const int fnam_second = view_row_order::rank(make_single("FNAM"), order);
	const int modl = view_row_order::rank(make_single("MODL"), order);
	const int scri = view_row_order::rank(make_single("SCRI"), order);

	REQUIRE(fnam_first == fnam_second);
	REQUIRE(fnam_first < modl);
	REQUIRE(modl < scri);
}
