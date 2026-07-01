#include <catch2/catch_all.hpp>
#include <merger/dict_merger.hpp>
#include <converter/script_parser.hpp>
#include <utility/tools.hpp>
#include <random>

namespace {

std::string random_multi_word_cell(std::mt19937 & rng)
{
	static const std::vector<std::string> prefixes = {
		"Ald", "Bal", "Tel", "Sadr", "Maar", "Gnar", "Hla", "Dren"
	};

	static const std::vector<std::string> suffixes = {
		"Velothi", "mora", "Fyr", "Gandosa", "Neen", "Plantation", "Mine", "Tower"
	};

	std::uniform_int_distribution<size_t> prefix_dist(0, prefixes.size() - 1);
	std::uniform_int_distribution<size_t> suffix_dist(0, suffixes.size() - 1);

	return prefixes[prefix_dist(rng)] + " " + suffixes[suffix_dist(rng)];
}

std::string random_translated_cell(std::mt19937 & rng)
{
	static const std::vector<std::string> prefixes = {
		"Nowy", "Stary", "Wielki", "Maly", "Gorny", "Dolny"
	};

	static const std::vector<std::string> suffixes = {
		"Zamek", "Wioska", "Kopalnia", "Plantacja", "Wieza", "Port"
	};

	std::uniform_int_distribution<size_t> prefix_dist(0, prefixes.size() - 1);
	std::uniform_int_distribution<size_t> suffix_dist(0, suffixes.size() - 1);

	return prefixes[prefix_dist(rng)] + " " + suffixes[suffix_dist(rng)];
}

} // namespace

TEST_CASE("script_parser_t, ShowMap unquoted cell replaced", "[u]")
{
	const auto seed = GENERATE(range(0, 100));
	std::mt19937 rng(static_cast<unsigned>(seed));

	const auto cell_name = random_multi_word_cell(rng);
	const auto translated = random_translated_cell(rng);

	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::cell, cell_name, translated);

	const std::string input = "ShowMap " + cell_name;
	const std::string expected = "ShowMap " + translated;

	script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t, PositionCell unquoted cell replaced", "[u]")
{
	const auto seed = GENERATE(range(0, 100));
	std::mt19937 rng(static_cast<unsigned>(seed));

	const auto cell_name = random_multi_word_cell(rng);
	const auto translated = random_translated_cell(rng);

	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::cell, cell_name, translated);

	const std::string input = "PositionCell, 0, 0, 0, 0, " + cell_name;
	const std::string expected = "PositionCell, 0, 0, 0, 0, " + translated;

	script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t, quoted cell not handled by unquoted path", "[u]")
{
	const auto seed = GENERATE(range(0, 100));
	std::mt19937 rng(static_cast<unsigned>(seed));

	const auto cell_name = random_multi_word_cell(rng);
	const auto translated = random_translated_cell(rng);

	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::cell, cell_name, translated);

	const std::string input = "ShowMap \"" + cell_name + "\"";
	const std::string expected = "ShowMap \"" + translated + "\"";

	script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == expected);
}

TEST_CASE("script_parser_t, ShowMap unquoted with comment stripped", "[u]")
{
	const auto seed = GENERATE(range(0, 100));
	std::mt19937 rng(static_cast<unsigned>(seed));

	const auto cell_name = random_multi_word_cell(rng);
	const auto translated = random_translated_cell(rng);

	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::cell, cell_name, translated);

	const std::string input = "ShowMap " + cell_name + " ; comment here";
	const std::string expected = "ShowMap " + translated + " ; comment here";

	script_parser_t parser(tools_t::rec_type_t::bnam, merger, "", "", input, "");

	REQUIRE(parser.get_new_script() == expected);
}
