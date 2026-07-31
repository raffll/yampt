#include <catch2/catch_test_macros.hpp>
#include <decoder/field_encoder.hpp>
#include <cstring>

TEST_CASE("field_encoder::encode_field, u8 encodes to 1 byte", "[u]")
{
	field_def_t field = {"val", field_type_t::u8, 0, 1, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "200", codepage_t::windows_1252, nullptr, 0};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 1);
	REQUIRE(static_cast<uint8_t>(result[0]) == 200);
}

TEST_CASE("field_encoder::encode_field, u16 encodes to 2 bytes LE", "[u]")
{
	field_def_t field = {"val", field_type_t::u16, 0, 2, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "1000", codepage_t::windows_1252, nullptr, 0};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 2);
	uint16_t value = 0;
	std::memcpy(&value, result.data(), 2);
	REQUIRE(value == 1000);
}

TEST_CASE("field_encoder::encode_field, u32 encodes to 4 bytes LE", "[u]")
{
	field_def_t field = {"val", field_type_t::u32, 0, 4, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "70000", codepage_t::windows_1252, nullptr, 0};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 4);
	uint32_t value = 0;
	std::memcpy(&value, result.data(), 4);
	REQUIRE(value == 70000);
}

TEST_CASE("field_encoder::encode_field, i8 encodes to 1 byte signed", "[u]")
{
	field_def_t field = {"val", field_type_t::i8, 0, 1, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "-5", codepage_t::windows_1252, nullptr, 0};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 1);
	int8_t value = 0;
	std::memcpy(&value, result.data(), 1);
	REQUIRE(value == -5);
}

TEST_CASE("field_encoder::encode_field, i32 encodes to 4 bytes signed", "[u]")
{
	field_def_t field = {"val", field_type_t::i32, 0, 4, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "-100000", codepage_t::windows_1252, nullptr, 0};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 4);
	int32_t value = 0;
	std::memcpy(&value, result.data(), 4);
	REQUIRE(value == -100000);
}

TEST_CASE("field_encoder::encode_field, f32 round-trip preserves value", "[u]")
{
	field_def_t field = {"val", field_type_t::f32, 0, 4, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "1.5", codepage_t::windows_1252, nullptr, 0};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 4);
	float value = 0.0f;
	std::memcpy(&value, result.data(), 4);
	REQUIRE(value == 1.5f);
}

TEST_CASE("field_encoder::encode_field, string_fixed null pads to field size", "[u]")
{
	field_def_t field = {"val", field_type_t::string_fixed, 0, 8, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "Hi", codepage_t::windows_1252, nullptr, 0};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 8);
	REQUIRE(result[0] == 'H');
	REQUIRE(result[1] == 'i');
	REQUIRE(result[2] == '\0');
	REQUIRE(result[7] == '\0');
}

TEST_CASE("field_encoder::encode_field, string_var appends null if original had it", "[u]")
{
	char existing[] = {'A', 'B', '\0'};
	field_def_t field = {"val", field_type_t::string_var, 0, 0, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "XY", codepage_t::windows_1252, existing, 3};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 3);
	REQUIRE(result[0] == 'X');
	REQUIRE(result[1] == 'Y');
	REQUIRE(result[2] == '\0');
}

TEST_CASE("field_encoder::encode_field, string_var no null if original lacked it", "[u]")
{
	char existing[] = {'A', 'B'};
	field_def_t field = {"val", field_type_t::string_var, 0, 0, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "XY", codepage_t::windows_1252, existing, 2};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 2);
	REQUIRE(result[0] == 'X');
	REQUIRE(result[1] == 'Y');
}

TEST_CASE("field_encoder::encode_field, enum name to LE index", "[u]")
{
	const char * names[] = {"Alpha", "Beta", "Gamma", nullptr};
	field_def_t field = {"val", field_type_t::enum_u16, 0, 2, names, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "Beta", codepage_t::windows_1252, nullptr, 0};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 2);
	uint16_t value = 0;
	std::memcpy(&value, result.data(), 2);
	REQUIRE(value == 1);
}

TEST_CASE("field_encoder::encode_field, bool_bit sets single bit", "[u]")
{
	char existing[] = {'\x00'};
	field_def_t field = {"val", field_type_t::bool_bit, 0, 2, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "Yes", codepage_t::windows_1252, existing, 1};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 1);
	REQUIRE(static_cast<uint8_t>(result[0]) == 0x04);
}

TEST_CASE("field_encoder::encode_field, bool_bit clears bit preserving adjacent", "[u]")
{
	char existing[] = {'\x07'};
	field_def_t field = {"val", field_type_t::bool_bit, 0, 1, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "No", codepage_t::windows_1252, existing, 1};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 1);
	REQUIRE(static_cast<uint8_t>(result[0]) == 0x05);
}

TEST_CASE("field_encoder::encode_field, flags preserves hidden bits", "[u]")
{
	const char * names[] = {"FlagA", "FlagB", nullptr};
	char existing[] = {'\x0C'};
	field_def_t field = {"val", field_type_t::flags_u8, 0, 1, nullptr, names, 2, nullptr};
	field_encoder::encode_context_t ctx{field, "FlagA", codepage_t::windows_1252, existing, 1};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 1);
	REQUIRE(static_cast<uint8_t>(result[0]) == 0x0D);
}

TEST_CASE("field_encoder::encode_field, binary hex string to bytes", "[u]")
{
	field_def_t field = {"val", field_type_t::binary, 0, 3, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "0A FF 3C", codepage_t::windows_1252, nullptr, 0};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 3);
	REQUIRE(static_cast<uint8_t>(result[0]) == 0x0A);
	REQUIRE(static_cast<uint8_t>(result[1]) == 0xFF);
	REQUIRE(static_cast<uint8_t>(result[2]) == 0x3C);
}

TEST_CASE("field_encoder::encode_field, raw hex string to bytes", "[u]")
{
	field_def_t field = {"val", field_type_t::raw, 0, 0, nullptr, nullptr, 0, nullptr};
	field_encoder::encode_context_t ctx{field, "0A FF 3C", codepage_t::windows_1252, nullptr, 0};
	auto result = field_encoder::encode_field(ctx);
	REQUIRE(result.size() == 3);
	REQUIRE(static_cast<uint8_t>(result[0]) == 0x0A);
	REQUIRE(static_cast<uint8_t>(result[1]) == 0xFF);
	REQUIRE(static_cast<uint8_t>(result[2]) == 0x3C);
}
