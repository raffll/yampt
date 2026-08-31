#include <catch2/catch_all.hpp>
#include <utility/case_fold.hpp>

#include <cstdint>

static int utf8_length(std::uint32_t code_point)
{
	if (code_point < 0x80)
		return 1;

	if (code_point < 0x800)
		return 2;

	if (code_point < 0x10000)
		return 3;

	return 4;
}

TEST_CASE("case_fold::to_lower, byte length preserved across covered ranges", "[u]")
{
	for (std::uint32_t code_point = 0x41; code_point <= 0x5A; ++code_point)
		REQUIRE(utf8_length(case_fold::to_lower(code_point)) == utf8_length(code_point));

	for (std::uint32_t code_point = 0x00C0; code_point <= 0x00DE; ++code_point)
	{
		if (code_point == 0x00D7)
			continue;

		REQUIRE(utf8_length(case_fold::to_lower(code_point)) == utf8_length(code_point));
	}

	for (std::uint32_t code_point = 0x0100; code_point <= 0x0177; ++code_point)
		REQUIRE(utf8_length(case_fold::to_lower(code_point)) == utf8_length(code_point));

	for (std::uint32_t code_point = 0x0139; code_point <= 0x0148; ++code_point)
		REQUIRE(utf8_length(case_fold::to_lower(code_point)) == utf8_length(code_point));

	for (std::uint32_t code_point = 0x0179; code_point <= 0x017E; ++code_point)
		REQUIRE(utf8_length(case_fold::to_lower(code_point)) == utf8_length(code_point));

	REQUIRE(utf8_length(case_fold::to_lower(0x0178)) == utf8_length(0x0178));

	for (std::uint32_t code_point = 0x0410; code_point <= 0x042F; ++code_point)
		REQUIRE(utf8_length(case_fold::to_lower(code_point)) == utf8_length(code_point));

	REQUIRE(utf8_length(case_fold::to_lower(0x0401)) == utf8_length(0x0401));
}

TEST_CASE("case_fold::to_lower, basic latin letters", "[u]")
{
	REQUIRE(case_fold::to_lower(0x0041) == 0x0061);
	REQUIRE(case_fold::to_lower(0x005A) == 0x007A);
}

TEST_CASE("case_fold::to_lower, polish characteristic letters", "[u]")
{
	REQUIRE(case_fold::to_lower(0x0104) == 0x0105);
	REQUIRE(case_fold::to_lower(0x0106) == 0x0107);
	REQUIRE(case_fold::to_lower(0x0118) == 0x0119);
	REQUIRE(case_fold::to_lower(0x0141) == 0x0142);
	REQUIRE(case_fold::to_lower(0x0143) == 0x0144);
	REQUIRE(case_fold::to_lower(0x00D3) == 0x00F3);
	REQUIRE(case_fold::to_lower(0x015A) == 0x015B);
	REQUIRE(case_fold::to_lower(0x0179) == 0x017A);
	REQUIRE(case_fold::to_lower(0x017B) == 0x017C);
}

TEST_CASE("case_fold::to_lower, german characteristic letters and sharp s passthrough", "[u]")
{
	REQUIRE(case_fold::to_lower(0x00C4) == 0x00E4);
	REQUIRE(case_fold::to_lower(0x00D6) == 0x00F6);
	REQUIRE(case_fold::to_lower(0x00DC) == 0x00FC);
	REQUIRE(case_fold::to_lower(0x00DF) == 0x00DF);
}

TEST_CASE("case_fold::to_lower, french characteristic letters", "[u]")
{
	REQUIRE(case_fold::to_lower(0x00C9) == 0x00E9);
	REQUIRE(case_fold::to_lower(0x00C8) == 0x00E8);
	REQUIRE(case_fold::to_lower(0x00C7) == 0x00E7);
	REQUIRE(case_fold::to_lower(0x00C0) == 0x00E0);
}

TEST_CASE("case_fold::to_lower, russian characteristic letters", "[u]")
{
	REQUIRE(case_fold::to_lower(0x0410) == 0x0430);
	REQUIRE(case_fold::to_lower(0x042F) == 0x044F);
	REQUIRE(case_fold::to_lower(0x0401) == 0x0451);
}

TEST_CASE("case_fold::to_lower, italian characteristic letters", "[u]")
{
	REQUIRE(case_fold::to_lower(0x00C0) == 0x00E0);
	REQUIRE(case_fold::to_lower(0x00C9) == 0x00E9);
	REQUIRE(case_fold::to_lower(0x00CC) == 0x00EC);
	REQUIRE(case_fold::to_lower(0x00D2) == 0x00F2);
	REQUIRE(case_fold::to_lower(0x00D9) == 0x00F9);
}

TEST_CASE("case_fold::to_lower, hungarian characteristic letters", "[u]")
{
	REQUIRE(case_fold::to_lower(0x0150) == 0x0151);
	REQUIRE(case_fold::to_lower(0x0170) == 0x0171);
	REQUIRE(case_fold::to_lower(0x00C1) == 0x00E1);
	REQUIRE(case_fold::to_lower(0x00C9) == 0x00E9);
	REQUIRE(case_fold::to_lower(0x00CD) == 0x00ED);
}
