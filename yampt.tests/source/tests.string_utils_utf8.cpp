#include <catch2/catch_all.hpp>
#include <utility/string_utils.hpp>

TEST_CASE("string_utils::to_lower_utf8, ascii lowercasing", "[u]")
{
	REQUIRE(string_utils::to_lower_utf8("Hello World") == "hello world");
	REQUIRE(string_utils::to_lower_utf8("ABCXYZ") == "abcxyz");
	REQUIRE(string_utils::to_lower_utf8("already lower") == "already lower");
	REQUIRE(string_utils::to_lower_utf8("Mix3d C4se!") == "mix3d c4se!");
	REQUIRE(string_utils::to_lower_utf8("") == "");
}

TEST_CASE("string_utils::to_lower_utf8, german umlauts", "[u]")
{
	REQUIRE(string_utils::to_lower_utf8("\xC3\x96") == "\xC3\xB6");
	REQUIRE(string_utils::to_lower_utf8("\xC3\x84") == "\xC3\xA4");
	REQUIRE(string_utils::to_lower_utf8("\xC3\x9C") == "\xC3\xBC");
}

TEST_CASE("string_utils::to_lower_utf8, french accents", "[u]")
{
	REQUIRE(string_utils::to_lower_utf8("\xC3\x89") == "\xC3\xA9");
	REQUIRE(string_utils::to_lower_utf8("\xC3\x88") == "\xC3\xA8");
	REQUIRE(string_utils::to_lower_utf8("\xC3\x87") == "\xC3\xA7");
}

TEST_CASE("string_utils::to_lower_utf8, balmora term preserves byte length", "[u]")
{
	const auto folded = string_utils::to_lower_utf8("B\xC4\x85lmora");
	REQUIRE(folded.size() == 8);
	REQUIRE(folded == "b\xC4\x85lmora");
}

TEST_CASE("string_utils::to_lower_utf8, polish letters", "[u]")
{
	REQUIRE(string_utils::to_lower_utf8("\xC5\x81") == "\xC5\x82");
	REQUIRE(string_utils::to_lower_utf8("\xC4\x84") == "\xC4\x85");
	REQUIRE(string_utils::to_lower_utf8("\xC4\x86") == "\xC4\x87");
	REQUIRE(string_utils::to_lower_utf8("\xC4\x98") == "\xC4\x99");
	REQUIRE(string_utils::to_lower_utf8("\xC5\x83") == "\xC5\x84");
	REQUIRE(string_utils::to_lower_utf8("\xC5\x9A") == "\xC5\x9B");
	REQUIRE(string_utils::to_lower_utf8("\xC5\xB9") == "\xC5\xBA");
	REQUIRE(string_utils::to_lower_utf8("\xC5\xBB") == "\xC5\xBC");
	REQUIRE(string_utils::to_lower_utf8("\xC3\x93") == "\xC3\xB3");
}

TEST_CASE("string_utils::to_lower_utf8, hungarian letters", "[u]")
{
	REQUIRE(string_utils::to_lower_utf8("\xC5\x90") == "\xC5\x91");
	REQUIRE(string_utils::to_lower_utf8("\xC5\xB0") == "\xC5\xB1");
}

TEST_CASE("string_utils::to_lower_utf8, cyrillic letters", "[u]")
{
	REQUIRE(string_utils::to_lower_utf8("\xD0\x90") == "\xD0\xB0");
	REQUIRE(string_utils::to_lower_utf8("\xD0\xAF") == "\xD1\x8F");
	REQUIRE(string_utils::to_lower_utf8("\xD0\x81") == "\xD1\x91");
}

TEST_CASE("string_utils::to_lower_utf8, sharp s passes through", "[u]")
{
	REQUIRE(string_utils::to_lower_utf8("\xC3\x9F") == "\xC3\x9F");
}

TEST_CASE("string_utils::to_lower_utf8, idempotent", "[u]")
{
	const std::string mixed = "B\xC4\x84LMORA \xD0\xAF \xC3\x96" "dsee";
	const auto once = string_utils::to_lower_utf8(mixed);
	REQUIRE(string_utils::to_lower_utf8(once) == once);
}

TEST_CASE("string_utils::to_lower_utf8, byte length preserved for covered ranges", "[u]")
{
	const std::string upper = "B\xC4\x84lmora \xD0\xAF\xC3\x96\xC5\x81";
	REQUIRE(string_utils::to_lower_utf8(upper).size() == upper.size());
}

TEST_CASE("string_utils::to_lower_utf8, non-letters unchanged", "[u]")
{
	REQUIRE(string_utils::to_lower_utf8("123 !@#$ .,;") == "123 !@#$ .,;");
	REQUIRE(string_utils::to_lower_utf8("\t\r\n") == "\t\r\n");
}

TEST_CASE("string_utils::to_lower_utf8, malformed utf8 does not crash", "[u]")
{
	REQUIRE(string_utils::to_lower_utf8(std::string("A\xC4", 2)) == std::string("a\xC4", 2));
	REQUIRE(string_utils::to_lower_utf8(std::string("\x85\x85", 2)) == std::string("\x85\x85", 2));
	REQUIRE(string_utils::to_lower_utf8(std::string("\xC4Z", 2)) == std::string("\xC4z", 2));
}

TEST_CASE("string_utils::case_insensitive_equal_utf8, accented", "[u]")
{
	REQUIRE(string_utils::case_insensitive_equal_utf8("B\xC4\x84lmora", "b\xC4\x85lmora") == true);
	REQUIRE(string_utils::case_insensitive_equal_utf8("\xD0\x9C\xD0\xBE", "\xD0\xBC\xD0\xBE") == true);
	REQUIRE(string_utils::case_insensitive_equal_utf8("\xC3\x96" "dsee", "\xC3\xB6" "dsee") == true);
	REQUIRE(string_utils::case_insensitive_equal_utf8("Balmora", "Vivec") == false);
}
