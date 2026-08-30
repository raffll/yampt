#include <catch2/catch_test_macros.hpp>
#include <decoder/field_validator.hpp>

static const char * enum_names[] = { "Red", "Green", "Blue", nullptr };
static const char * flag_names[] = { "Flag_A", "Flag_B", "_hidden", nullptr };

TEST_CASE("field_validator::validate_field, u8 boundaries", "[u]")
{
	field_def_t field = { "val", field_type_t::u8, 0, 1, nullptr, nullptr, 0, nullptr };

	SECTION("zero is valid")
	{
		auto result = field_validator::validate_field(field, "0", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("255 is valid")
	{
		auto result = field_validator::validate_field(field, "255", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("256 is out of range")
	{
		auto result = field_validator::validate_field(field, "256", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
	SECTION("empty input rejected")
	{
		auto result = field_validator::validate_field(field, "", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
}

TEST_CASE("field_validator::validate_field, u16 boundaries", "[u]")
{
	field_def_t field = { "val", field_type_t::u16, 0, 2, nullptr, nullptr, 0, nullptr };

	SECTION("zero is valid")
	{
		auto result = field_validator::validate_field(field, "0", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("65535 is valid")
	{
		auto result = field_validator::validate_field(field, "65535", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("65536 is out of range")
	{
		auto result = field_validator::validate_field(field, "65536", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
}

TEST_CASE("field_validator::validate_field, i8 boundaries", "[u]")
{
	field_def_t field = { "val", field_type_t::i8, 0, 1, nullptr, nullptr, 0, nullptr };

	SECTION("-128 is valid")
	{
		auto result = field_validator::validate_field(field, "-128", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("127 is valid")
	{
		auto result = field_validator::validate_field(field, "127", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("128 is out of range")
	{
		auto result = field_validator::validate_field(field, "128", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
}

TEST_CASE("field_validator::validate_field, float validation", "[u]")
{
	field_def_t field = { "val", field_type_t::f32, 0, 4, nullptr, nullptr, 0, nullptr };

	SECTION("valid number")
	{
		auto result = field_validator::validate_field(field, "3.14", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("NaN text rejected")
	{
		auto result = field_validator::validate_field(field, "NaN", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
	SECTION("Inf text rejected")
	{
		auto result = field_validator::validate_field(field, "Inf", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
	SECTION("non-numeric text rejected")
	{
		auto result = field_validator::validate_field(field, "abc", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
	SECTION("empty input rejected")
	{
		auto result = field_validator::validate_field(field, "", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
}

TEST_CASE("field_validator::validate_field, string_fixed size limits", "[u]")
{
	field_def_t field = { "val", field_type_t::string_fixed, 0, 4, nullptr, nullptr, 0, nullptr };

	SECTION("within size")
	{
		auto result = field_validator::validate_field(field, "Hi", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("exceeds after codepage encode")
	{
		auto result = field_validator::validate_field(field, "ABCDE", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
}

TEST_CASE("field_validator::validate_field, enum names", "[u]")
{
	field_def_t field = { "val", field_type_t::enum_u8, 0, 1, enum_names, nullptr, 0, nullptr };

	SECTION("valid name accepted")
	{
		auto result = field_validator::validate_field(field, "Green", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("invalid name rejected")
	{
		auto result = field_validator::validate_field(field, "Yellow", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
}

TEST_CASE("field_validator::validate_field, flags validation", "[u]")
{
	field_def_t field = { "val", field_type_t::flags_u32, 0, 4, nullptr, flag_names, 3, nullptr };

	SECTION("separated valid names")
	{
		auto result = field_validator::validate_field(field, "Flag_A | Flag_B", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("invalid name rejected")
	{
		auto result = field_validator::validate_field(field, "Flag_A | Unknown", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
	SECTION("empty string as zero")
	{
		auto result = field_validator::validate_field(field, "", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("hex format accepted")
	{
		auto result = field_validator::validate_field(field, "0x0000000F", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
}

TEST_CASE("field_validator::validate_field, bool_bit strict values", "[u]")
{
	field_def_t field = { "val", field_type_t::bool_bit, 0, 1, nullptr, nullptr, 0, nullptr };

	SECTION("Yes accepted")
	{
		auto result = field_validator::validate_field(field, "Yes", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("No accepted")
	{
		auto result = field_validator::validate_field(field, "No", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("lowercase yes rejected")
	{
		auto result = field_validator::validate_field(field, "yes", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
	SECTION("TRUE rejected")
	{
		auto result = field_validator::validate_field(field, "TRUE", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
}

TEST_CASE("field_validator::validate_field, binary hex pairs", "[u]")
{
	field_def_t field = { "val", field_type_t::binary, 0, 3, nullptr, nullptr, 0, nullptr };

	SECTION("valid hex pairs matching field size")
	{
		auto result = field_validator::validate_field(field, "0A FF 3C", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("wrong byte count rejected")
	{
		auto result = field_validator::validate_field(field, "0A FF", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
	SECTION("invalid hex chars rejected")
	{
		auto result = field_validator::validate_field(field, "0A GG 3C", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
}

TEST_CASE("field_validator::validate_field, raw hex pairs with existing_sub_size", "[u]")
{
	field_def_t field = { "val", field_type_t::raw, 0, 0, nullptr, nullptr, 0, nullptr };

	SECTION("valid hex pairs matching existing_sub_size")
	{
		auto result = field_validator::validate_field(field, "AA BB", codepage_t::windows_1252, 2);
		REQUIRE(result.valid);
	}
	SECTION("wrong byte count rejected")
	{
		auto result = field_validator::validate_field(field, "AA BB CC", codepage_t::windows_1252, 2);
		REQUIRE_FALSE(result.valid);
	}
}

TEST_CASE("field_validator::validate_field, signed integer with enum_names", "[u]")
{
	field_def_t field = { "val", field_type_t::i8, 0, 1, enum_names, nullptr, 0, nullptr };

	SECTION("valid enum name accepted")
	{
		auto result = field_validator::validate_field(field, "Red", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("None accepted as -1")
	{
		auto result = field_validator::validate_field(field, "None", codepage_t::windows_1252, 0);
		REQUIRE(result.valid);
	}
	SECTION("invalid name rejected")
	{
		auto result = field_validator::validate_field(field, "Purple", codepage_t::windows_1252, 0);
		REQUIRE_FALSE(result.valid);
	}
}
