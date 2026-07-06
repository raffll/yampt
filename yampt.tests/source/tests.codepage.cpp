#include <catch2/catch_all.hpp>
#include <io/codepage.hpp>

TEST_CASE("codepage_t::decode_to_utf8, empty string", "[u]")
{
	REQUIRE(decode_to_utf8("", codepage_t::windows_1250).empty());
	REQUIRE(decode_to_utf8("", codepage_t::windows_1251).empty());
	REQUIRE(decode_to_utf8("", codepage_t::windows_1252).empty());
}

TEST_CASE("codepage_t::decode_to_utf8, ASCII passthrough", "[u]")
{
	std::string ascii = "Hello World 123";
	REQUIRE(decode_to_utf8(ascii, codepage_t::windows_1250) == ascii);
	REQUIRE(decode_to_utf8(ascii, codepage_t::windows_1251) == ascii);
	REQUIRE(decode_to_utf8(ascii, codepage_t::windows_1252) == ascii);
}

TEST_CASE("codepage_t::decode_to_utf8, Polish characters from 1250", "[u]")
{
	// Ä… = 0xB9 in Windows-1250, UTF-8: C4 85
	std::string raw(1, '\xB9');
	std::string utf8 = decode_to_utf8(raw, codepage_t::windows_1250);
	REQUIRE(utf8 == "\xC4\x85");

	// Ĺş = 0x9F in Windows-1250, UTF-8: C5 BA
	raw = std::string(1, '\x9F');
	utf8 = decode_to_utf8(raw, codepage_t::windows_1250);
	REQUIRE(utf8 == "\xC5\xBA");

	// Ĺ› = 0x9C in Windows-1250, UTF-8: C5 9B
	raw = std::string(1, '\x9C');
	utf8 = decode_to_utf8(raw, codepage_t::windows_1250);
	REQUIRE(utf8 == "\xC5\x9B");
}

TEST_CASE("codepage_t::encode_from_utf8, empty string", "[u]")
{
	REQUIRE(encode_from_utf8("", codepage_t::windows_1250).empty());
	REQUIRE(encode_from_utf8("", codepage_t::windows_1251).empty());
	REQUIRE(encode_from_utf8("", codepage_t::windows_1252).empty());
}

TEST_CASE("codepage_t::encode_from_utf8, ASCII passthrough", "[u]")
{
	std::string ascii = "Hello World 123";
	REQUIRE(encode_from_utf8(ascii, codepage_t::windows_1250) == ascii);
	REQUIRE(encode_from_utf8(ascii, codepage_t::windows_1251) == ascii);
	REQUIRE(encode_from_utf8(ascii, codepage_t::windows_1252) == ascii);
}

TEST_CASE("codepage_t::decode_to_utf8, round-trip 1250", "[u]")
{
	// Full Polish sentence with diacritics
	std::string raw_1250 = "Zdr\xF3j wody"; // "ZdrĂłj wody" (Ăł = 0xF3 in 1250)
	std::string utf8 = decode_to_utf8(raw_1250, codepage_t::windows_1250);
	std::string back = encode_from_utf8(utf8, codepage_t::windows_1250);
	REQUIRE(back == raw_1250);
}

TEST_CASE("codepage_t::decode_to_utf8, round-trip 1251", "[u]")
{
	// Cyrillic: "ĐźŃ€Đ¸Đ˛ĐµŃ‚" = CF F0 E8 E2 E5 F2 in Windows-1251
	std::string raw_1251 = "\xCF\xF0\xE8\xE2\xE5\xF2";
	std::string utf8 = decode_to_utf8(raw_1251, codepage_t::windows_1251);
	std::string back = encode_from_utf8(utf8, codepage_t::windows_1251);
	REQUIRE(back == raw_1251);
}

TEST_CASE("codepage_t::decode_to_utf8, round-trip 1252", "[u]")
{
	// French: "cafĂ©" = 63 61 66 E9 in Windows-1252 (Ă© = 0xE9)
	std::string raw_1252 = "caf\xE9";
	std::string utf8 = decode_to_utf8(raw_1252, codepage_t::windows_1252);
	std::string back = encode_from_utf8(utf8, codepage_t::windows_1252);
	REQUIRE(back == raw_1252);
}

TEST_CASE("codepage_t::codepage_name, all codepages", "[u]")
{
	REQUIRE(std::string(codepage_name(codepage_t::windows_1250)) == "Windows-1250 (Central European)");
	REQUIRE(std::string(codepage_name(codepage_t::windows_1251)) == "Windows-1251 (Cyrillic)");
	REQUIRE(std::string(codepage_name(codepage_t::windows_1252)) == "Windows-1252 (Western)");
}
