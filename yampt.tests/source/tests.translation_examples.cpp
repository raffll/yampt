#include <catch2/catch_all.hpp>
#include <translator/translation_example_ops.hpp>

TEST_CASE("translation_example_ops::format_examples_prompt, empty list yields empty string", "[u]")
{
	const std::vector<translation_example_t> examples;

	const auto prompt = translation_example_ops::format_examples_prompt(examples);

	REQUIRE(prompt.empty());
}

TEST_CASE("translation_example_ops::format_examples_prompt, one example yields labeled block", "[u]")
{
	const std::vector<translation_example_t> examples{{"Nerevarine", "Nerevaryn"}};

	const auto prompt = translation_example_ops::format_examples_prompt(examples);

	REQUIRE(prompt.find("Examples:") != std::string::npos);
	REQUIRE(prompt.find("Nerevarine") != std::string::npos);
	REQUIRE(prompt.find("Nerevaryn") != std::string::npos);
}

TEST_CASE("translation_example_ops::format_examples_prompt, three examples preserve order", "[u]")
{
	const std::vector<translation_example_t> examples{
	    {"Balmora", "Balmora"},
	    {"Vivec", "Vivek"},
	    {"Ald-ruhn", "Ald-ruhn"}};

	const auto prompt = translation_example_ops::format_examples_prompt(examples);

	const auto pos_first = prompt.find("Balmora");
	const auto pos_second = prompt.find("Vivec");
	const auto pos_third = prompt.find("Ald-ruhn");

	REQUIRE(pos_first != std::string::npos);
	REQUIRE(pos_second != std::string::npos);
	REQUIRE(pos_third != std::string::npos);
	REQUIRE(pos_first < pos_second);
	REQUIRE(pos_second < pos_third);
}

TEST_CASE("translation_example_ops::add_capped, adding to a full list is rejected", "[u]")
{
	const std::vector<translation_example_t> examples{
	    {"one", "1"},
	    {"two", "2"},
	    {"three", "3"}};
	const std::vector<translation_example_t> pairs{{"four", "4"}};

	const auto result = translation_example_ops::add_capped(examples, pairs);

	REQUIRE(result.size() == 3);
	REQUIRE_FALSE(translation_example_ops::contains_original(result, "four"));
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
	const std::vector<translation_example_t> examples{{"one", "1"}};
	const std::vector<translation_example_t> pairs{
	    {"two", "2"},
	    {"three", "3"},
	    {"four", "4"},
	    {"five", "5"}};

	const auto result = translation_example_ops::add_capped(examples, pairs);

	REQUIRE(result.size() == 3);
	REQUIRE(result[0].original == "one");
	REQUIRE(result[1].original == "two");
	REQUIRE(result[2].original == "three");
	REQUIRE_FALSE(translation_example_ops::contains_original(result, "four"));
	REQUIRE_FALSE(translation_example_ops::contains_original(result, "five"));
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
