#include <catch2/catch_all.hpp>
#include <rapidcheck/catch.h>
#include <rapidcheck.h>
#include <creator/cell_matcher.hpp>
#include <regex>

TEST_CASE("cell_matcher_t::is_interior_cell, flags bit check", "[u][pbt]")
{
	rc::prop(
		"result matches bit 0 of first byte",
		[]()
	{
		auto data_content = *rc::gen::arbitrary<std::string>();
		while (data_content.size() < 4)
			data_content += '\0';

		const auto result = cell_matcher_t::is_interior_cell(data_content);
		unsigned char flags_byte = static_cast<unsigned char>(data_content[0]);
		const auto expected = (flags_byte & 0x01) != 0;
		RC_ASSERT(result == expected);
	});
}

TEST_CASE("cell_matcher_t::make_exterior_coord_key, deterministic", "[u][pbt]")
{
	rc::prop(
		"same input always produces same output",
		[]()
	{
		auto data_content = *rc::gen::arbitrary<std::string>();
		while (data_content.size() < 12)
			data_content += '\0';

		const auto result_1 = cell_matcher_t::make_exterior_coord_key(data_content);
		const auto result_2 = cell_matcher_t::make_exterior_coord_key(data_content);
		RC_ASSERT(result_1 == result_2);
	});
}

TEST_CASE("cell_matcher_t::make_exterior_coord_key, format", "[u][pbt]")
{
	rc::prop(
		"result matches GRID[int,int] format",
		[]()
	{
		auto data_content = *rc::gen::arbitrary<std::string>();
		while (data_content.size() < 12)
			data_content += '\0';

		const auto result = cell_matcher_t::make_exterior_coord_key(data_content);
		RC_ASSERT(!result.empty());
		std::regex pattern(R"(GRID\[-?\d+,-?\d+\])");
		RC_ASSERT(std::regex_match(result, pattern));
	});
}

TEST_CASE("cell_matcher_t::make_exterior_coord_key, short input", "[u][pbt]")
{
	rc::prop(
		"strings shorter than 12 bytes return empty",
		[]()
	{
		const auto length = *rc::gen::inRange(0, 12);
		auto data_content = *rc::gen::arbitrary<std::string>();
		data_content.resize(length);

		const auto result = cell_matcher_t::make_exterior_coord_key(data_content);
		RC_ASSERT(result.empty());
	});
}

TEST_CASE("cell_matcher_t::make_cell_key_text, deterministic", "[u][pbt]")
{
	rc::prop(
		"same input always produces same output",
		[]()
	{
		const auto fingerprint = *rc::gen::arbitrary<std::string>();
		const auto result_1 = cell_matcher_t::make_cell_key_text(fingerprint);
		const auto result_2 = cell_matcher_t::make_cell_key_text(fingerprint);
		RC_ASSERT(result_1 == result_2);
	});
}

TEST_CASE("cell_matcher_t::make_cell_key_text, fixed length", "[u][pbt]")
{
	rc::prop(
		"output is always exactly 16 hex characters",
		[]()
	{
		const auto fingerprint = *rc::gen::arbitrary<std::string>();
		const auto result = cell_matcher_t::make_cell_key_text(fingerprint);
		RC_ASSERT(result.size() == 16);
		for (char c : result)
			RC_ASSERT((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
	});
}
