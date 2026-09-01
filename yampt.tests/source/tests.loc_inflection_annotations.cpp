#include <catch2/catch_all.hpp>
#include <editor/loc_inflection_annotations.hpp>

TEST_CASE("loc_inflection_annotations::build, maps top entries to inflection_form", "[u]")
{
	const std::vector<loc_types::loc_entry_t> entries { { "miecze", "miecz" }, { "mieczy", "miecz" } };

	const auto result = loc_inflection_annotations::build(loc_types::loc_file_kind_t::top, entries);

	REQUIRE(result.size() == 2);
	REQUIRE(result[0].kind == annotation_t::inflection_form);
	REQUIRE(result[0].old_text == "miecze");
	REQUIRE(result[0].new_text == "miecz");
	REQUIRE(result[1].old_text == "mieczy");
	REQUIRE(result[1].new_text == "miecz");
}

TEST_CASE("loc_inflection_annotations::build, maps mrk entries to inflection_form", "[u]")
{
	const std::vector<loc_types::loc_entry_t> entries { { "keyword", "Topic" } };

	const auto result = loc_inflection_annotations::build(loc_types::loc_file_kind_t::mrk, entries);

	REQUIRE(result.size() == 1);
	REQUIRE(result[0].kind == annotation_t::inflection_form);
	REQUIRE(result[0].old_text == "keyword");
	REQUIRE(result[0].new_text == "Topic");
}

TEST_CASE("loc_inflection_annotations::build, cel entries produce nothing", "[u]")
{
	const std::vector<loc_types::loc_entry_t> entries { { "Balmora", "Balmora PL" } };

	const auto result = loc_inflection_annotations::build(loc_types::loc_file_kind_t::cel, entries);

	REQUIRE(result.empty());
}

TEST_CASE("loc_inflection_annotations::build, empty entries produce nothing", "[u]")
{
	const auto result = loc_inflection_annotations::build(loc_types::loc_file_kind_t::top, {});

	REQUIRE(result.empty());
}
