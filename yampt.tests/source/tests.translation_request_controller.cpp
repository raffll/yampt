#include <catch2/catch_all.hpp>
#include <editor/glossary_line_preparer.hpp>
#include <string>
#include <vector>

TEST_CASE("glossary_line_preparer::prepare_lines, empty lines stay empty", "[i]")
{
	const std::vector<std::string> source_lines { "alpha", "", "beta" };

	const auto prepared = glossary_line_preparer::prepare_lines(
	    source_lines, [](const std::string & source) { return source + "_x"; });

	REQUIRE(prepared.size() == 3);
	REQUIRE(prepared[0] == "alpha_x");
	REQUIRE(prepared[1] == std::string());
	REQUIRE(prepared[2] == "beta_x");
}

TEST_CASE("glossary_line_preparer::prepare_lines, preserves order", "[i]")
{
	const std::vector<std::string> source_lines { "one", "two", "three" };

	const auto prepared = glossary_line_preparer::prepare_lines(
	    source_lines, [](const std::string & source) { return source; });

	REQUIRE(prepared == source_lines);
}

TEST_CASE("glossary_line_preparer::prepare_lines, empty input yields empty output", "[i]")
{
	const std::vector<std::string> source_lines;

	const auto prepared = glossary_line_preparer::prepare_lines(
	    source_lines, [](const std::string & source) { return source + "_x"; });

	REQUIRE(prepared.empty());
}
