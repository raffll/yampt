#include <catch2/catch_test_macros.hpp>
#include <decoder/field_encoder.hpp>

#include <cstdint>
#include <cstring>
#include <string>

static std::string build_fixed_record()
{
	std::string content;
	content.append("TEST", 4);
	uint32_t rec_size = 12;
	content.append(reinterpret_cast<const char *>(&rec_size), 4);
	content.append(4, '\0');
	content.append(4, '\0');
	content.append("DATA", 4);
	uint32_t sub_size = 4;
	content.append(reinterpret_cast<const char *>(&sub_size), 4);
	content.append("\x01\x02\x03\x04", 4);
	return content;
}

static std::string build_string_record(const std::string & text)
{
	std::string content;
	content.append("TEST", 4);
	uint32_t rec_size = static_cast<uint32_t>(8 + text.size());
	content.append(reinterpret_cast<const char *>(&rec_size), 4);
	content.append(4, '\0');
	content.append(4, '\0');
	content.append("NAME", 4);
	uint32_t sub_size = static_cast<uint32_t>(text.size());
	content.append(reinterpret_cast<const char *>(&sub_size), 4);
	content.append(text);
	return content;
}

TEST_CASE("field_encoder::patch_sub_record, fixed-size splice at correct offset", "[u]")
{
	auto content = build_fixed_record();
	field_def_t field = { "val", field_type_t::u32, 0, 4, nullptr, nullptr, 0, nullptr };

	uint32_t new_value = 0xDEADBEEF;
	std::string encoded(4, '\0');
	std::memcpy(encoded.data(), &new_value, 4);

	auto patched = field_encoder::patch_sub_record(content, 16, field, encoded);

	uint32_t result = 0;
	std::memcpy(&result, patched.data() + 24, 4);
	REQUIRE(result == 0xDEADBEEF);

	REQUIRE(patched.substr(0, 16) == content.substr(0, 16));
	REQUIRE(patched.substr(16, 8) == content.substr(16, 8));
}

TEST_CASE("field_encoder::patch_sub_record, string_var grow", "[u]")
{
	auto content = build_string_record("Hi");
	field_def_t field = { "name", field_type_t::string_var, 0, 0, nullptr, nullptr, 0, nullptr };

	std::string encoded = "Hello";

	auto patched = field_encoder::patch_sub_record(content, 16, field, encoded);

	uint32_t new_sub_size = 0;
	std::memcpy(&new_sub_size, patched.data() + 20, 4);
	REQUIRE(new_sub_size == 5);

	REQUIRE(patched.size() == 16 + 8 + 5);
	REQUIRE(patched.substr(24, 5) == "Hello");
}

TEST_CASE("field_encoder::patch_sub_record, string_var shrink", "[u]")
{
	auto content = build_string_record("LongText");
	field_def_t field = { "name", field_type_t::string_var, 0, 0, nullptr, nullptr, 0, nullptr };

	std::string encoded = "Ab";

	auto patched = field_encoder::patch_sub_record(content, 16, field, encoded);

	uint32_t new_sub_size = 0;
	std::memcpy(&new_sub_size, patched.data() + 20, 4);
	REQUIRE(new_sub_size == 2);

	REQUIRE(patched.size() == 16 + 8 + 2);
	REQUIRE(patched.substr(24, 2) == "Ab");
}

TEST_CASE("field_encoder::patch_record_size, recalculates header correctly", "[u]")
{
	auto content = build_fixed_record();
	content.append("XTRA", 4);
	uint32_t extra_size = 2;
	content.append(reinterpret_cast<const char *>(&extra_size), 4);
	content.append("\xAA\xBB", 2);

	auto patched = field_encoder::patch_record_size(content);

	uint32_t stored_size = 0;
	std::memcpy(&stored_size, patched.data() + 4, 4);
	REQUIRE(stored_size == patched.size() - 16);
}
