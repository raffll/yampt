#include <catch2/catch_all.hpp>
#include <model/table_columns.hpp>

TEST_CASE("table_columns_t::columns_for_kind, dict has all five columns", "[u]")
{
	const auto columns = table_columns_t::columns_for_kind(document_kind_t::dict);
	REQUIRE(columns == std::vector<table_col_t> { col_id, col_key, col_original, col_translation, col_status });
}

TEST_CASE("table_columns_t::columns_for_kind, eet has all five columns", "[u]")
{
	const auto columns = table_columns_t::columns_for_kind(document_kind_t::eet);
	REQUIRE(columns == std::vector<table_col_t> { col_id, col_key, col_original, col_translation, col_status });
}

TEST_CASE("table_columns_t::columns_for_kind, plugin has all five columns", "[u]")
{
	const auto columns = table_columns_t::columns_for_kind(document_kind_t::plugin);
	REQUIRE(columns == std::vector<table_col_t> { col_id, col_key, col_original, col_translation, col_status });
}

TEST_CASE("table_columns_t::columns_for_kind, yaml omits id and key", "[u]")
{
	const auto columns = table_columns_t::columns_for_kind(document_kind_t::yaml);
	REQUIRE(columns == std::vector<table_col_t> { col_original, col_translation, col_status });
}

TEST_CASE("table_columns_t::columns_for_kind, loc omits id, key, and status", "[u]")
{
	const auto columns = table_columns_t::columns_for_kind(document_kind_t::loc);
	REQUIRE(columns == std::vector<table_col_t> { col_original, col_translation });
}

TEST_CASE("table_columns_t::count, reflects kind", "[u]")
{
	REQUIRE(table_columns_t(document_kind_t::dict).count() == 5);
	REQUIRE(table_columns_t(document_kind_t::yaml).count() == 3);
	REQUIRE(table_columns_t(document_kind_t::loc).count() == 2);
}

TEST_CASE("table_columns_t::at, dict position maps to logical column", "[u]")
{
	const table_columns_t columns(document_kind_t::dict);
	REQUIRE(columns.at(0) == col_id);
	REQUIRE(columns.at(1) == col_key);
	REQUIRE(columns.at(2) == col_original);
	REQUIRE(columns.at(3) == col_translation);
	REQUIRE(columns.at(4) == col_status);
}

TEST_CASE("table_columns_t::at, yaml position maps to logical column", "[u]")
{
	const table_columns_t columns(document_kind_t::yaml);
	REQUIRE(columns.at(0) == col_original);
	REQUIRE(columns.at(1) == col_translation);
	REQUIRE(columns.at(2) == col_status);
}

TEST_CASE("table_columns_t::at, out of range returns col_count", "[u]")
{
	const table_columns_t columns(document_kind_t::yaml);
	REQUIRE(columns.at(-1) == col_count);
	REQUIRE(columns.at(3) == col_count);
	REQUIRE(columns.at(99) == col_count);
}

TEST_CASE("table_columns_t::position_of, dict returns visual position", "[u]")
{
	const table_columns_t columns(document_kind_t::dict);
	REQUIRE(columns.position_of(col_id) == 0);
	REQUIRE(columns.position_of(col_translation) == 3);
	REQUIRE(columns.position_of(col_status) == 4);
}

TEST_CASE("table_columns_t::position_of, yaml shifts translation and status", "[u]")
{
	const table_columns_t columns(document_kind_t::yaml);
	REQUIRE(columns.position_of(col_original) == 0);
	REQUIRE(columns.position_of(col_translation) == 1);
	REQUIRE(columns.position_of(col_status) == 2);
}

TEST_CASE("table_columns_t::position_of, absent column returns minus one", "[u]")
{
	const table_columns_t columns(document_kind_t::yaml);
	REQUIRE(columns.position_of(col_id) == -1);
	REQUIRE(columns.position_of(col_key) == -1);
}

TEST_CASE("table_columns_t::contains, presence per kind", "[u]")
{
	const table_columns_t dict_columns(document_kind_t::dict);
	REQUIRE(dict_columns.contains(col_key) == true);
	REQUIRE(dict_columns.contains(col_id) == true);

	const table_columns_t yaml_columns(document_kind_t::yaml);
	REQUIRE(yaml_columns.contains(col_key) == false);
	REQUIRE(yaml_columns.contains(col_id) == false);
	REQUIRE(yaml_columns.contains(col_translation) == true);
}

TEST_CASE("table_columns_t::set_for_kind, switches layout", "[u]")
{
	table_columns_t columns(document_kind_t::dict);
	REQUIRE(columns.count() == 5);

	columns.set_for_kind(document_kind_t::loc);
	REQUIRE(columns.count() == 2);
	REQUIRE(columns.contains(col_key) == false);
	REQUIRE(columns.contains(col_status) == false);

	columns.set_for_kind(document_kind_t::dict);
	REQUIRE(columns.count() == 5);
	REQUIRE(columns.contains(col_key) == true);
}

TEST_CASE("table_columns_t, default constructor is dict layout", "[u]")
{
	const table_columns_t columns;
	REQUIRE(columns.count() == 5);
	REQUIRE(columns.at(0) == col_id);
}
