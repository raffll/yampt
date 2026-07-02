#include <catch2/catch_all.hpp>
#include <decoder/view_tree_format.hpp>
#include <cstring>

TEST_CASE("decode_field, enum_u8 shows name only", "[u]")
{
	static const char * const names[] = { "Head", "Cuirass", "Shield", nullptr };
	field_def_t field { "Type", field_type_t::enum_u8, 0, 1, names, nullptr, 0 };

	char data[1] = { 1 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "Cuirass");
}

TEST_CASE("decode_field, enum_u8 unknown value shows number", "[u]")
{
	static const char * const names[] = { "Head", "Cuirass", nullptr };
	field_def_t field { "Type", field_type_t::enum_u8, 0, 1, names, nullptr, 0 };

	char data[1] = { 5 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "5");
}

TEST_CASE("decode_field, enum_u32 shows name only", "[u]")
{
	static const char * const names[] = { "Helmet", "Cuirass", "Pauldron", nullptr };
	field_def_t field { "Type", field_type_t::enum_u32, 0, 4, names, nullptr, 0 };

	char data[4] = {};
	uint32_t val = 2;
	std::memcpy(data, &val, 4);
	auto result = decode_field(field, data, 4);
	REQUIRE(result == "Pauldron");
}

TEST_CASE("decode_field, flags_u32 shows names only", "[u]")
{
	static const char * const flag_names[] = { "Interior", "Water", "Sleep", nullptr };
	field_def_t field { "Flags", field_type_t::flags_u32, 0, 4, nullptr, flag_names, 3 };

	char data[4] = {};
	uint32_t val = 0x03;
	std::memcpy(data, &val, 4);
	auto result = decode_field(field, data, 4);
	REQUIRE(result == "Interior | Water");
}

TEST_CASE("decode_field, flags_u32 no flags set shows hex", "[u]")
{
	static const char * const flag_names[] = { "Interior", "Water", nullptr };
	field_def_t field { "Flags", field_type_t::flags_u32, 0, 4, nullptr, flag_names, 2 };

	char data[4] = {};
	auto result = decode_field(field, data, 4);
	REQUIRE(result == "0x00000000");
}

TEST_CASE("decode_field, i8 with enum shows name only", "[u]")
{
	static const char * const names[] = { "None", "Male", "Female", nullptr };
	field_def_t field { "Gender", field_type_t::i8, 0, 1, names, nullptr, 0 };

	char data[1] = { 2 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "Female");
}

TEST_CASE("decode_field, i8 with enum -1 shows None", "[u]")
{
	static const char * const names[] = { "Zero", "One", nullptr };
	field_def_t field { "Type", field_type_t::i8, 0, 1, names, nullptr, 0 };

	char data[1];
	int8_t val = -1;
	std::memcpy(data, &val, 1);
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "None");
}

TEST_CASE("decode_field, i8 without enum shows number", "[u]")
{
	field_def_t field { "Value", field_type_t::i8, 0, 1, nullptr, nullptr, 0 };

	char data[1] = { 42 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "42");
}
