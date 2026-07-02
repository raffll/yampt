#include <catch2/catch_all.hpp>
#include <decoder/sub_record_schema.hpp>
#include <string>

TEST_CASE("sub_record_schema_t::find_schema, CELL NAM5 lookup", "[u]")
{
	const auto * schema = find_schema("CELL", "NAM5", 4);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 4);
}

TEST_CASE("sub_record_schema_t::find_schema, CELL FLTV lookup", "[u]")
{
	const auto * schema = find_schema("CELL", "FLTV", 4);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
}

TEST_CASE("sub_record_schema_t::find_schema, CELL NAM9 lookup", "[u]")
{
	const auto * schema = find_schema("CELL", "NAM9", 4);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
}

TEST_CASE("sub_record_schema_t::find_schema, GLOB FNAM lookup", "[u]")
{
	const auto * schema = find_schema("GLOB", "FNAM", 1);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
}

TEST_CASE("sub_record_schema_t::find_schema, GLOB FLTV lookup", "[u]")
{
	const auto * schema = find_schema("GLOB", "FLTV", 4);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
}

TEST_CASE("sub_record_schema_t::find_schema, SNDG DATA lookup", "[u]")
{
	const auto * schema = find_schema("SNDG", "DATA", 4);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
	REQUIRE(std::string(schema->fields[0].enum_names[0]) == "Left Foot");
	REQUIRE(std::string(schema->fields[0].enum_names[7]) == "Land");
}

TEST_CASE("sub_record_schema_t::find_schema, LAND DATA lookup", "[u]")
{
	const auto * schema = find_schema("LAND", "DATA", 4);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
}

TEST_CASE("sub_record_schema_t::find_schema, AI_W field count", "[u]")
{
	const auto * schema = find_schema("*", "AI_W", 14);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 12);
}

TEST_CASE("sub_record_schema_t::find_schema, AI_T field count", "[u]")
{
	const auto * schema = find_schema("*", "AI_T", 16);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 4);
}

TEST_CASE("sub_record_schema_t::find_schema, AI_F field count", "[u]")
{
	const auto * schema = find_schema("*", "AI_F", 48);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 6);
}

TEST_CASE("sub_record_schema_t::find_schema, AI_A field count", "[u]")
{
	const auto * schema = find_schema("*", "AI_A", 33);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 2);
}

TEST_CASE("sub_record_schema_t::find_schema, DODT wildcard", "[u]")
{
	const auto * schema = find_schema("NPC_", "DODT", 24);
	REQUIRE(schema != nullptr);
}

TEST_CASE("sub_record_schema_t::find_schema, CELL DATA size 12", "[u]")
{
	const auto * schema = find_schema("CELL", "DATA", 12);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 3);
	REQUIRE(std::string(schema->fields[0].name) == "Flags");
}

TEST_CASE("sub_record_schema_t::find_schema, CELL DATA size 24", "[u]")
{
	const auto * schema = find_schema("CELL", "DATA", 24);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 6);
	REQUIRE(std::string(schema->fields[0].name) == "X Pos");
}

TEST_CASE("sub_record_schema_t::find_schema, LAND VHGT excluded", "[u]")
{
	const auto * schema = find_schema("LAND", "VHGT", 4225);
	REQUIRE(schema == nullptr);
}
