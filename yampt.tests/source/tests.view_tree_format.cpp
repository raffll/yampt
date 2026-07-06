#include <catch2/catch_all.hpp>
#include <decoder/view_tree_format.hpp>
#include <cstring>

TEST_CASE("view_tree_format::decode_field, enum_u8 shows name only", "[u]")
{
	static const char * const names[] = { "Head", "Cuirass", "Shield", nullptr };
	field_def_t field { "Type", field_type_t::enum_u8, 0, 1, names, nullptr, 0 };

	char data[1] = { 1 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "Cuirass");
}

TEST_CASE("view_tree_format::decode_field, enum_u8 unknown value shows number", "[u]")
{
	static const char * const names[] = { "Head", "Cuirass", nullptr };
	field_def_t field { "Type", field_type_t::enum_u8, 0, 1, names, nullptr, 0 };

	char data[1] = { 5 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "5");
}

TEST_CASE("view_tree_format::decode_field, enum_u32 shows name only", "[u]")
{
	static const char * const names[] = { "Helmet", "Cuirass", "Pauldron", nullptr };
	field_def_t field { "Type", field_type_t::enum_u32, 0, 4, names, nullptr, 0 };

	char data[4] = {};
	uint32_t val = 2;
	std::memcpy(data, &val, 4);
	auto result = decode_field(field, data, 4);
	REQUIRE(result == "Pauldron");
}

TEST_CASE("view_tree_format::decode_field, flags_u32 shows names only", "[u]")
{
	static const char * const flag_names[] = { "Interior", "Water", "Sleep", nullptr };
	field_def_t field { "Flags", field_type_t::flags_u32, 0, 4, nullptr, flag_names, 3 };

	char data[4] = {};
	uint32_t val = 0x03;
	std::memcpy(data, &val, 4);
	auto result = decode_field(field, data, 4);
	REQUIRE(result == "Interior | Water");
}

TEST_CASE("view_tree_format::decode_field, flags_u32 no flags set shows hex", "[u]")
{
	static const char * const flag_names[] = { "Interior", "Water", nullptr };
	field_def_t field { "Flags", field_type_t::flags_u32, 0, 4, nullptr, flag_names, 2 };

	char data[4] = {};
	auto result = decode_field(field, data, 4);
	REQUIRE(result == "0x00000000");
}

TEST_CASE("view_tree_format::decode_field, i8 with enum shows name only", "[u]")
{
	static const char * const names[] = { "None", "Male", "Female", nullptr };
	field_def_t field { "Gender", field_type_t::i8, 0, 1, names, nullptr, 0 };

	char data[1] = { 2 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "Female");
}

TEST_CASE("view_tree_format::decode_field, i8 with enum -1 shows None", "[u]")
{
	static const char * const names[] = { "Zero", "One", nullptr };
	field_def_t field { "Type", field_type_t::i8, 0, 1, names, nullptr, 0 };

	char data[1];
	int8_t val = -1;
	std::memcpy(data, &val, 1);
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "None");
}

TEST_CASE("view_tree_format::decode_field, i8 without enum shows number", "[u]")
{
	field_def_t field { "Value", field_type_t::i8, 0, 1, nullptr, nullptr, 0 };

	char data[1] = { 42 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "42");
}

TEST_CASE("view_tree_format::decode_field, bool_bit 0 set shows Yes", "[u]")
{
	field_def_t field { "Female", field_type_t::bool_bit, 0, 0, nullptr, nullptr, 0 };

	char data[1] = { 0x01 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "Yes");
}

TEST_CASE("view_tree_format::decode_field, bool_bit 0 unset shows No", "[u]")
{
	field_def_t field { "Female", field_type_t::bool_bit, 0, 0, nullptr, nullptr, 0 };

	char data[1] = { 0x00 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "No");
}

TEST_CASE("view_tree_format::decode_field, bool_bit 1 set shows Yes", "[u]")
{
	field_def_t field { "Playable", field_type_t::bool_bit, 0, 1, nullptr, nullptr, 0 };

	char data[1] = { 0x02 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "Yes");
}

TEST_CASE("view_tree_format::decode_field, bool_bit 1 unset shows No", "[u]")
{
	field_def_t field { "Playable", field_type_t::bool_bit, 0, 1, nullptr, nullptr, 0 };

	char data[1] = { 0x01 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "No");
}

TEST_CASE("view_tree_format::decode_field, bool_bit both bits set", "[u]")
{
	field_def_t female { "Female", field_type_t::bool_bit, 0, 0, nullptr, nullptr, 0 };
	field_def_t playable { "Playable", field_type_t::bool_bit, 0, 1, nullptr, nullptr, 0 };

	char data[1] = { 0x03 };
	REQUIRE(decode_field(female, data, 1) == "Yes");
	REQUIRE(decode_field(playable, data, 1) == "Yes");
}

TEST_CASE("view_tree_format::decode_field, bool_bit with offset", "[u]")
{
	field_def_t field { "Female", field_type_t::bool_bit, 2, 0, nullptr, nullptr, 0 };

	char data[4] = { 0x00, 0x00, 0x01, 0x00 };
	auto result = decode_field(field, data, 4);
	REQUIRE(result == "Yes");
}

TEST_CASE("view_tree_format::decode_field, bool_bit out of bounds returns empty", "[u]")
{
	field_def_t field { "Female", field_type_t::bool_bit, 5, 0, nullptr, nullptr, 0 };

	char data[4] = { 0x01, 0x01, 0x01, 0x01 };
	auto result = decode_field(field, data, 4);
	REQUIRE(result == "");
}

TEST_CASE("view_tree_format::decode_field, flags_u8 value zero shows hex", "[u]")
{
	static const char * const flag_names[] = { "Female", "Playable", nullptr };
	field_def_t field { "Flags", field_type_t::flags_u8, 0, 1, nullptr, flag_names, 2 };

	char data[1] = { 0x00 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "0x00000000");
}

TEST_CASE("view_tree_format::decode_field, flags_u8 single flag shows name", "[u]")
{
	static const char * const flag_names[] = { "Female", "Playable", nullptr };
	field_def_t field { "Flags", field_type_t::flags_u8, 0, 1, nullptr, flag_names, 2 };

	char data[1] = { 0x02 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "Playable");
}

TEST_CASE("view_tree_format::decode_field, i8 value -1 with empty enum shows None", "[u]")
{
	static const char * const names[] = { nullptr };
	field_def_t field { "Rank", field_type_t::i8, 0, 1, names, nullptr, 0 };

	char data[1];
	int8_t val = -1;
	std::memcpy(data, &val, 1);
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "None");
}

TEST_CASE("view_tree_format::decode_field, i8 value 5 with empty enum shows number", "[u]")
{
	static const char * const names[] = { nullptr };
	field_def_t field { "Rank", field_type_t::i8, 0, 1, names, nullptr, 0 };

	char data[1] = { 5 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "5");
}

TEST_CASE("view_tree_format::decode_field, enum_u32 shows Yes for scroll", "[u]")
{
	static const char * const names[] = { "No", "Yes", nullptr };
	field_def_t field { "Scroll", field_type_t::enum_u32, 0, 4, names, nullptr, 0 };

	char data[4] = {};
	uint32_t val = 1;
	std::memcpy(data, &val, 4);
	auto result = decode_field(field, data, 4);
	REQUIRE(result == "Yes");
}

TEST_CASE("view_tree_format::decode_field, enum_u32 shows No for non-scroll", "[u]")
{
	static const char * const names[] = { "No", "Yes", nullptr };
	field_def_t field { "Scroll", field_type_t::enum_u32, 0, 4, names, nullptr, 0 };

	char data[4] = {};
	auto result = decode_field(field, data, 4);
	REQUIRE(result == "No");
}

TEST_CASE("view_tree_format::decode_field, i8 gender Male", "[u]")
{
	static const char * const names[] = { "Male", "Female", nullptr };
	field_def_t field { "Gender", field_type_t::i8, 0, 1, names, nullptr, 0 };

	char data[1] = { 0 };
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "Male");
}

TEST_CASE("view_tree_format::decode_field, i8 gender None for 0xFF", "[u]")
{
	static const char * const names[] = { "Male", "Female", nullptr };
	field_def_t field { "Gender", field_type_t::i8, 0, 1, names, nullptr, 0 };

	char data[1];
	int8_t val = -1;
	std::memcpy(data, &val, 1);
	auto result = decode_field(field, data, 1);
	REQUIRE(result == "None");
}
