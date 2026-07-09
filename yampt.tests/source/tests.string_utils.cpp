#include <catch2/catch_all.hpp>
#include <utility/string_utils.hpp>

TEST_CASE("string_utils::to_lower, ascii mixed case", "[u]")
{
	REQUIRE(string_utils::to_lower("Hello World") == "hello world");
	REQUIRE(string_utils::to_lower("ABCXYZ") == "abcxyz");
	REQUIRE(string_utils::to_lower("already") == "already");
	REQUIRE(string_utils::to_lower("") == "");
}

TEST_CASE("string_utils::normalize_path, backslash to forward slash", "[u]")
{
	REQUIRE(string_utils::normalize_path("C:\\Users\\test\\file.txt") == "C:/Users/test/file.txt");
	REQUIRE(string_utils::normalize_path("no/change") == "no/change");
	REQUIRE(string_utils::normalize_path("") == "");
	REQUIRE(string_utils::normalize_path("\\\\server\\share") == "//server/share");
}

TEST_CASE("string_utils::extract_filename, various paths", "[u]")
{
	REQUIRE(string_utils::extract_filename("C:/Users/test/file.txt") == "file.txt");
	REQUIRE(string_utils::extract_filename("C:\\data\\dict.json") == "dict.json");
	REQUIRE(string_utils::extract_filename("file.txt") == "file.txt");
	REQUIRE(string_utils::extract_filename("") == "");
	REQUIRE(string_utils::extract_filename("/root/") == "");
}

TEST_CASE("string_utils::trim, whitespace removal", "[u]")
{
	REQUIRE(string_utils::trim("  hello  ") == "hello");
	REQUIRE(string_utils::trim("\t\r\nspaced\r\n") == "spaced");
	REQUIRE(string_utils::trim("nospace") == "nospace");
	REQUIRE(string_utils::trim("") == "");
	REQUIRE(string_utils::trim("   ") == "");
}

TEST_CASE("string_utils::utf8_byte_to_char_offset, ascii only", "[u]")
{
	REQUIRE(string_utils::utf8_byte_to_char_offset("hello", 0) == 0);
	REQUIRE(string_utils::utf8_byte_to_char_offset("hello", 3) == 3);
	REQUIRE(string_utils::utf8_byte_to_char_offset("hello", 5) == 5);
}

TEST_CASE("string_utils::utf8_byte_to_char_offset, multi-byte chars", "[u]")
{
	const std::string polish = "\xC4\x85\xC4\x87\xC4\x99";
	REQUIRE(string_utils::utf8_byte_to_char_offset(polish, 0) == 0);
	REQUIRE(string_utils::utf8_byte_to_char_offset(polish, 2) == 1);
	REQUIRE(string_utils::utf8_byte_to_char_offset(polish, 4) == 2);
	REQUIRE(string_utils::utf8_byte_to_char_offset(polish, 6) == 3);
}

TEST_CASE("string_utils::replace_non_printable_with_dot, mixed content", "[u]")
{
	REQUIRE(string_utils::replace_non_printable_with_dot("hello") == "hello");
	REQUIRE(string_utils::replace_non_printable_with_dot("a\x01""b") == "a.b");
	REQUIRE(string_utils::replace_non_printable_with_dot(std::string("\x00\x1F", 2)) == "..");
	REQUIRE(string_utils::replace_non_printable_with_dot("") == "");
}
