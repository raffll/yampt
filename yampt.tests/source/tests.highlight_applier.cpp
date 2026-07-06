#include <catch2/catch_all.hpp>
#include <utility/string_utils.hpp>

TEST_CASE("string_utils::utf8_byte_to_char_offset, pure ascii", "[u]")
{
	const std::string text = "hello world";
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 0) == 0);
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 5) == 5);
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 11) == 11);
}

TEST_CASE("string_utils::utf8_byte_to_char_offset, two-byte chars", "[u]")
{
	const std::string text = "Popie\xC5\x82na K\xC5\x82\xC4\x85twa: Mana";
	const int byte_of_m = 20;
	const int char_of_m = 17;
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, byte_of_m) == char_of_m);
}

TEST_CASE("string_utils::utf8_byte_to_char_offset, offset zero", "[u]")
{
	const std::string text = "\xC5\x82\xC4\x85\xC4\x99";
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 0) == 0);
}

TEST_CASE("string_utils::utf8_byte_to_char_offset, multi-byte at start", "[u]")
{
	const std::string text = "\xC5\x82"
	                         "abc";
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 2) == 1);
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 3) == 2);
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 5) == 4);
}

TEST_CASE("string_utils::utf8_byte_to_char_offset, three-byte chars", "[u]")
{
	const std::string text = "\xE2\x80\x93"
	                         "ab";
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 3) == 1);
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 4) == 2);
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 5) == 3);
}

TEST_CASE("string_utils::utf8_byte_to_char_offset, beyond text length", "[u]")
{
	const std::string text = "abc";
	REQUIRE(string_utils::utf8_byte_to_char_offset(text, 10) == 3);
}
