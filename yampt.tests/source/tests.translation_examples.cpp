#include <catch2/catch_all.hpp>
#include <translator/translation_example_ops.hpp>

TEST_CASE("translation_example_ops::format_examples_lines, empty list yields empty string", "[u]")
{
	const std::vector<translation_example_t> examples;

	const auto lines = translation_example_ops::format_examples_lines(examples);

	REQUIRE(lines.empty());
}

TEST_CASE("translation_example_ops::format_examples_lines, one example yields a line", "[u]")
{
	const std::vector<translation_example_t> examples{{"Nerevarine", "Nerevaryn"}};

	const auto lines = translation_example_ops::format_examples_lines(examples);

	REQUIRE(lines == "Nerevarine -> Nerevaryn");
}

TEST_CASE("translation_example_ops::format_examples_lines, three examples preserve order", "[u]")
{
	const std::vector<translation_example_t> examples{
	    {"Balmora", "Balmora"},
	    {"Vivec", "Vivek"},
	    {"Ald-ruhn", "Ald-ruhn"}};

	const auto lines = translation_example_ops::format_examples_lines(examples);

	const auto pos_first = lines.find("Balmora");
	const auto pos_second = lines.find("Vivec");
	const auto pos_third = lines.find("Ald-ruhn");

	REQUIRE(pos_first != std::string::npos);
	REQUIRE(pos_second != std::string::npos);
	REQUIRE(pos_third != std::string::npos);
	REQUIRE(pos_first < pos_second);
	REQUIRE(pos_second < pos_third);
}

TEST_CASE("translation_example_ops::add_capped, adding to a full list is rejected", "[u]")
{
	std::vector<translation_example_t> examples;
	for (int index = 0; index < max_examples; ++index)
		examples.push_back({ "original_" + std::to_string(index), std::to_string(index) });

	const std::vector<translation_example_t> pairs{{"overflow", "x"}};

	const auto result = translation_example_ops::add_capped(examples, pairs);

	REQUIRE(result.size() == static_cast<size_t>(max_examples));
	REQUIRE_FALSE(translation_example_ops::contains_original(result, "overflow"));
}

TEST_CASE("translation_example_ops::add_capped, duplicate original is rejected", "[u]")
{
	const std::vector<translation_example_t> examples{{"one", "1"}};
	const std::vector<translation_example_t> pairs{{"one", "different"}};

	const auto result = translation_example_ops::add_capped(examples, pairs);

	REQUIRE(result.size() == 1);
	REQUIRE(result[0].translation == "1");
}

TEST_CASE("translation_example_ops::add_capped, multi-add stops at cap", "[u]")
{
	std::vector<translation_example_t> examples;
	for (int index = 0; index < max_examples - 2; ++index)
		examples.push_back({ "seed_" + std::to_string(index), std::to_string(index) });

	const std::vector<translation_example_t> pairs{
	    {"fits_one", "a"},
	    {"fits_two", "b"},
	    {"overflow_one", "c"},
	    {"overflow_two", "d"}};

	const auto result = translation_example_ops::add_capped(examples, pairs);

	REQUIRE(result.size() == static_cast<size_t>(max_examples));
	REQUIRE(translation_example_ops::contains_original(result, "fits_one"));
	REQUIRE(translation_example_ops::contains_original(result, "fits_two"));
	REQUIRE_FALSE(translation_example_ops::contains_original(result, "overflow_one"));
	REQUIRE_FALSE(translation_example_ops::contains_original(result, "overflow_two"));
}

TEST_CASE("translation_example_ops::remove_by_original, removes existing original", "[u]")
{
	const std::vector<translation_example_t> examples{
	    {"one", "1"},
	    {"two", "2"},
	    {"three", "3"}};

	const auto result = translation_example_ops::remove_by_original(examples, "two");

	REQUIRE(result.size() == 2);
	REQUIRE(result[0].original == "one");
	REQUIRE(result[1].original == "three");
	REQUIRE_FALSE(translation_example_ops::contains_original(result, "two"));
}

TEST_CASE("translation_example_ops::remove_by_original, absent original is a no-op", "[u]")
{
	const std::vector<translation_example_t> examples{
	    {"one", "1"},
	    {"two", "2"}};

	const auto result = translation_example_ops::remove_by_original(examples, "missing");

	REQUIRE(result.size() == 2);
	REQUIRE(result[0].original == "one");
	REQUIRE(result[1].original == "two");
}

TEST_CASE("translation_example_ops::contains_original, true when present", "[u]")
{
	const std::vector<translation_example_t> examples{
	    {"one", "1"},
	    {"two", "2"}};

	REQUIRE(translation_example_ops::contains_original(examples, "two"));
}

TEST_CASE("translation_example_ops::contains_original, false when absent", "[u]")
{
	const std::vector<translation_example_t> examples{{"one", "1"}};

	REQUIRE_FALSE(translation_example_ops::contains_original(examples, "absent"));
}
