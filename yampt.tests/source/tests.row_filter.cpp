#include <catch2/catch_all.hpp>
#include <editor/row_filter.hpp>

namespace {

table_row_t make_row(const std::string & key, const std::string & old_text, const std::string & new_text)
{
	table_row_t row;
	row.key_text = key;
	row.old_text = old_text;
	row.new_text = new_text;
	row.type = rec_type_t::info;
	row.status = status_t::translated;
	return row;
}

} // namespace

TEST_CASE("row_filter_t::matches, empty query matches everything", "[u]")
{
	row_filter_t filter;
	filter.set_config({});
	REQUIRE(filter.matches(make_row("key", "old", "new")) == true);
}

TEST_CASE("row_filter_t::matches, case insensitive substring", "[u]")
{
	row_filter_t filter;
	filter.set_config({ .query = "balmora", .case_sensitive = false });

	REQUIRE(filter.matches(make_row("cell_Balmora", "Balmora", "Balmora")) == true);
	REQUIRE(filter.matches(make_row("cell_Vivec", "Vivec", "Vivec")) == false);
}

TEST_CASE("row_filter_t::matches, case sensitive substring", "[u]")
{
	row_filter_t filter;
	filter.set_config({ .query = "Balmora", .case_sensitive = true });

	REQUIRE(filter.matches(make_row("cell_Balmora", "Balmora", "Balmora")) == true);
	REQUIRE(filter.matches(make_row("cell_balmora", "balmora", "balmora")) == false);
}

TEST_CASE("row_filter_t::matches, column restriction to original", "[u]")
{
	row_filter_t filter;
	filter.set_config({ .query = "needle", .columns = { search_column_t::original } });

	REQUIRE(filter.matches(make_row("needle_key", "haystack", "needle_trans")) == false);
	REQUIRE(filter.matches(make_row("some_key", "has needle here", "other")) == true);
}

TEST_CASE("row_filter_t::matches, column restriction to translation", "[u]")
{
	row_filter_t filter;
	filter.set_config({ .query = "translated", .columns = { search_column_t::translation } });

	REQUIRE(filter.matches(make_row("key", "translated text", "nope")) == false);
	REQUIRE(filter.matches(make_row("key", "original", "fully translated")) == true);
}

TEST_CASE("row_filter_t::matches, regex mode basic pattern", "[u]")
{
	row_filter_t filter;
	filter.set_config({ .query = "^cell_", .regex_mode = true });

	REQUIRE(filter.matches(make_row("cell_Balmora", "Balmora", "Balmora")) == true);
	REQUIRE(filter.matches(make_row("fnam_Balmora", "Balmora", "Balmora")) == false);
}

TEST_CASE("row_filter_t::matches, regex mode case insensitive", "[u]")
{
	row_filter_t filter;
	filter.set_config({ .query = "BALMORA", .case_sensitive = false, .regex_mode = true });

	REQUIRE(filter.matches(make_row("key", "balmora city", "trans")) == true);
}

TEST_CASE("row_filter_t::matches, invalid regex returns false", "[u]")
{
	row_filter_t filter;
	filter.set_config({ .query = "[invalid(", .regex_mode = true });

	REQUIRE(filter.matches(make_row("key", "[invalid(", "trans")) == false);
}

TEST_CASE("row_filter_t::has_query, empty and non-empty", "[u]")
{
	row_filter_t filter;

	filter.set_config({});
	REQUIRE(filter.has_query() == false);

	filter.set_config({ .query = "test" });
	REQUIRE(filter.has_query() == true);
}
