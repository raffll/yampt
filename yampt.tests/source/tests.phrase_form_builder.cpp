#include <catch2/catch_all.hpp>
#include <creator/phrase_form_builder.hpp>
#include <algorithm>
#include <string>
#include <vector>

namespace {

std::string case_suffix_of(const std::string & word)
{
	const auto underscore = word.rfind('_');
	if (underscore == std::string::npos)
		return "";

	return word.substr(underscore + 1);
}

bool all_words_agree(const std::vector<std::string> & words)
{
	if (words.empty())
		return false;

	const auto reference = case_suffix_of(words.front());
	for (const auto & word : words)
	{
		if (case_suffix_of(word) != reference)
			return false;
	}

	return true;
}

bool contains(const std::vector<std::string> & forms, const std::string & value)
{
	return std::find(forms.begin(), forms.end(), value) != forms.end();
}

} // namespace

TEST_CASE("phrase_form_builder::build_phrase_forms, agreeing combination is produced", "[u]")
{
	const std::vector<std::vector<std::string>> per_word = {
	    { "silver_nom", "silver_gen" },
	    { "sword_nom", "sword_gen" },
	};

	const auto forms = phrase_form_builder::build_phrase_forms(per_word, all_words_agree, 50);

	REQUIRE(contains(forms, "silver_nom sword_nom"));
	REQUIRE(contains(forms, "silver_gen sword_gen"));
}

TEST_CASE("phrase_form_builder::build_phrase_forms, one-word-only invalid mixes are absent", "[u]")
{
	const std::vector<std::vector<std::string>> per_word = {
	    { "silver_nom", "silver_gen" },
	    { "sword_nom", "sword_gen" },
	};

	const auto forms = phrase_form_builder::build_phrase_forms(per_word, all_words_agree, 50);

	REQUIRE_FALSE(contains(forms, "silver_gen sword_nom"));
	REQUIRE_FALSE(contains(forms, "silver_nom sword_gen"));
}

TEST_CASE("phrase_form_builder::build_phrase_forms, per-phrase cap is respected", "[u]")
{
	std::vector<std::string> first_word;
	std::vector<std::string> second_word;
	for (int index = 0; index < 20; ++index)
	{
		first_word.push_back("a" + std::to_string(index));
		second_word.push_back("b" + std::to_string(index));
	}

	const std::vector<std::vector<std::string>> per_word = { first_word, second_word };

	const auto accept_all = [](const std::vector<std::string> &) { return true; };

	const auto forms = phrase_form_builder::build_phrase_forms(per_word, accept_all, 50);

	REQUIRE(forms.size() <= 50);
}

TEST_CASE("phrase_form_builder::build_phrase_forms, invalid combinations do not consume the cap", "[u]")
{
	const std::vector<std::vector<std::string>> per_word = {
	    { "silver_nom", "silver_gen" },
	    { "sword_nom", "sword_gen" },
	};

	const auto forms = phrase_form_builder::build_phrase_forms(per_word, all_words_agree, 50);

	REQUIRE(forms.size() == 2);
}
