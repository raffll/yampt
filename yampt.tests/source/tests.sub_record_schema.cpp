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

TEST_CASE("sub_record_schema_t::find_schema, ARMO INDX 1-byte biped parts", "[u]")
{
	const auto * schema = find_schema("ARMO", "INDX", 1);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
	REQUIRE(schema->fields[0].enum_names != nullptr);
	REQUIRE(std::string(schema->fields[0].enum_names[0]) == "Head");
	REQUIRE(std::string(schema->fields[0].enum_names[3]) == "Cuirass");
	REQUIRE(std::string(schema->fields[0].enum_names[5]) == "Skirt");
	REQUIRE(std::string(schema->fields[0].enum_names[23]) == "Right Pauldron");
	REQUIRE(std::string(schema->fields[0].enum_names[24]) == "Left Pauldron");
	REQUIRE(std::string(schema->fields[0].enum_names[26]) == "Tail");
	REQUIRE(schema->fields[0].enum_names[27] == nullptr);
}

TEST_CASE("sub_record_schema_t::find_schema, CLOT INDX 1-byte uses same biped parts", "[u]")
{
	const auto * schema = find_schema("CLOT", "INDX", 1);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
	REQUIRE(std::string(schema->fields[0].enum_names[6]) == "Right Hand");
	REQUIRE(std::string(schema->fields[0].enum_names[10]) == "Shield");
}

TEST_CASE("sub_record_schema_t::find_schema, generic INDX 4-byte has no enum", "[u]")
{
	const auto * schema = find_schema("SPEL", "INDX", 4);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
	REQUIRE(schema->fields[0].enum_names == nullptr);
}

TEST_CASE("sub_record_schema_t::find_schema, BODY BYDT has 5 fields from 4 bytes", "[u]")
{
	const auto * schema = find_schema("BODY", "BYDT", 4);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 5);
	REQUIRE(std::string(schema->fields[0].name) == "Part");
	REQUIRE(std::string(schema->fields[1].name) == "Vampire");
	REQUIRE(std::string(schema->fields[2].name) == "Female");
	REQUIRE(schema->fields[2].type == field_type_t::bool_bit);
	REQUIRE(schema->fields[2].offset == 2);
	REQUIRE(schema->fields[2].size == 0);
	REQUIRE(std::string(schema->fields[3].name) == "Playable");
	REQUIRE(schema->fields[3].type == field_type_t::bool_bit);
	REQUIRE(schema->fields[3].offset == 2);
	REQUIRE(schema->fields[3].size == 1);
	REQUIRE(std::string(schema->fields[4].name) == "Part Type");
}

TEST_CASE("sub_record_schema_t::find_schema, SPEL ENAM 24-byte matches", "[u]")
{
	const auto * schema = find_schema("SPEL", "ENAM", 24);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count > 0);
	REQUIRE(std::string(schema->fields[0].name) == "Effect ID");
}

TEST_CASE("sub_record_schema_t::find_schema, CLOT ENAM does not match effect schema", "[u]")
{
	const auto * schema = find_schema("CLOT", "ENAM", 24);
	REQUIRE(schema == nullptr);
}

TEST_CASE("sub_record_schema_t::find_schema, ARMO ENAM does not match effect schema", "[u]")
{
	const auto * schema = find_schema("ARMO", "ENAM", 24);
	REQUIRE(schema == nullptr);
}

TEST_CASE("sub_record_schema_t::find_schema, ENCH ENAM 24-byte matches", "[u]")
{
	const auto * schema = find_schema("ENCH", "ENAM", 24);
	REQUIRE(schema != nullptr);
	REQUIRE(std::string(schema->fields[0].name) == "Effect ID");
}

TEST_CASE("sub_record_schema_t::find_schema, INFO DATA has 5 fields", "[u]")
{
	const auto * schema = find_schema("INFO", "DATA", 12);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 5);
	REQUIRE(std::string(schema->fields[0].name) == "Type");
	REQUIRE(schema->fields[0].type == field_type_t::enum_u32);
	REQUIRE(std::string(schema->fields[2].name) == "Rank");
	REQUIRE(schema->fields[2].type == field_type_t::i8);
	REQUIRE(std::string(schema->fields[3].name) == "Gender");
	REQUIRE(schema->fields[3].type == field_type_t::i8);
	REQUIRE(std::string(schema->fields[4].name) == "PC Rank");
}

TEST_CASE("sub_record_schema_t::find_schema, QSTF 1-byte quest marker", "[u]")
{
	const auto * schema = find_schema("INFO", "QSTF", 1);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
	REQUIRE(std::string(schema->fields[0].name) == "Value");
	REQUIRE(schema->fields[0].enum_names != nullptr);
	REQUIRE(std::string(schema->fields[0].enum_names[0]) == "No");
	REQUIRE(std::string(schema->fields[0].enum_names[1]) == "Yes");
}

TEST_CASE("sub_record_schema_t::find_schema, QSTN 1-byte quest marker", "[u]")
{
	const auto * schema = find_schema("INFO", "QSTN", 1);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
}

TEST_CASE("sub_record_schema_t::find_schema, QSTR 1-byte quest marker", "[u]")
{
	const auto * schema = find_schema("INFO", "QSTR", 1);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 1);
}

TEST_CASE("sub_record_schema_t::find_schema, BOOK BKDT scroll field is enum_u32", "[u]")
{
	const auto * schema = find_schema("BOOK", "BKDT", 20);
	REQUIRE(schema != nullptr);
	REQUIRE(schema->field_count == 5);
	REQUIRE(std::string(schema->fields[2].name) == "Scroll");
	REQUIRE(schema->fields[2].type == field_type_t::enum_u32);
	REQUIRE(schema->fields[2].enum_names != nullptr);
	REQUIRE(std::string(schema->fields[2].enum_names[0]) == "No");
	REQUIRE(std::string(schema->fields[2].enum_names[1]) == "Yes");
	REQUIRE(std::string(schema->fields[4].name) == "Enchant Points");
}

TEST_CASE("sub_record_schema_t::find_schema, CLAS CLDT playable is bool_bit", "[u]")
{
	const auto * schema = find_schema("CLAS", "CLDT", 60);
	REQUIRE(schema != nullptr);
	bool found_playable = false;
	bool found_services = false;
	for (size_t i = 0; i < schema->field_count; ++i)
	{
		if (std::string(schema->fields[i].name) == "Playable")
		{
			REQUIRE(schema->fields[i].type == field_type_t::bool_bit);
			REQUIRE(schema->fields[i].offset == 52);
			REQUIRE(schema->fields[i].size == 0);
			found_playable = true;
		}
		if (std::string(schema->fields[i].name) == "Services")
		{
			REQUIRE(schema->fields[i].type == field_type_t::flags_u32);
			found_services = true;
		}
	}
	REQUIRE(found_playable);
	REQUIRE(found_services);
}
