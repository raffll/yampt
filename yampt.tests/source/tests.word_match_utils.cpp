#include <catch2/catch_all.hpp>
#include <creator/word_match_utils.hpp>
#include <rapidcheck/catch.h>
#include <algorithm>
#include <rapidcheck.h>
#include <set>

TEST_CASE("word_match_utils::split_words, alphanumeric tokens", "[u][pbt]")
{
	rc::prop(
	    "all tokens non-empty lowercase alphanumeric",
	    []()
	{
		const auto text = *rc::gen::arbitrary<std::string>();
		const auto words = split_words(text);
		for (const auto & w : words)
		{
			RC_ASSERT(!w.empty());
			for (char c : w)
				RC_ASSERT((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'));
		}
	});
}

TEST_CASE("word_match_utils::split_words, no content lost", "[u][pbt]")
{
	rc::prop(
	    "concatenation equals lowercase alphanumeric content",
	    []()
	{
		const auto text = *rc::gen::arbitrary<std::string>();
		const auto words = split_words(text);

		std::string concatenated;
		for (const auto & w : words)
			concatenated += w;

		std::string expected;
		for (char c : text)
		{
			if (std::isalnum(static_cast<unsigned char>(c)))
				expected += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}

		RC_ASSERT(concatenated == expected);
	});
}

TEST_CASE("word_match_utils::count_shared_words, bounded", "[u][pbt]")
{
	rc::prop(
	    "result between 0 and source size",
	    []()
	{
		const auto source = *rc::gen::arbitrary<std::vector<std::string>>();
		const auto target = *rc::gen::arbitrary<std::vector<std::string>>();
		const auto count = count_shared_words(source, target);
		RC_ASSERT(count >= 0);
		RC_ASSERT(count <= static_cast<int>(source.size()));
	});
}

TEST_CASE("word_match_utils::count_shared_words, self identity", "[u][pbt]")
{
	rc::prop(
	    "unique vector against itself equals its size",
	    []()
	{
		auto words = *rc::gen::arbitrary<std::vector<std::string>>();
		std::set<std::string> seen;
		std::vector<std::string> unique;
		for (const auto & w : words)
		{
			if (!w.empty() && seen.insert(w).second)
				unique.push_back(w);
		}

		RC_PRE(!unique.empty());
		const auto count = count_shared_words(unique, unique);
		RC_ASSERT(count == static_cast<int>(unique.size()));
	});
}

TEST_CASE("word_match_utils::build_compare_words, superset", "[u][pbt]")
{
	rc::prop(
	    "result contains all translated and original words",
	    []()
	{
		const auto translated = *rc::gen::arbitrary<std::vector<std::string>>();
		const auto original = *rc::gen::arbitrary<std::vector<std::string>>();
		const auto result = build_compare_words(translated, original);

		for (const auto & w : translated)
			RC_ASSERT(std::find(result.begin(), result.end(), w) != result.end());

		for (const auto & w : original)
			RC_ASSERT(std::find(result.begin(), result.end(), w) != result.end());
	});
}

TEST_CASE("word_match_utils::build_compare_words, no duplicates", "[u][pbt]")
{
	rc::prop(
	    "result has no duplicate entries given unique inputs",
	    []()
	{
		auto translated_raw = *rc::gen::arbitrary<std::vector<std::string>>();
		auto original_raw = *rc::gen::arbitrary<std::vector<std::string>>();

		std::set<std::string> seen_t;
		std::vector<std::string> translated;
		for (const auto & w : translated_raw)
		{
			if (seen_t.insert(w).second)
				translated.push_back(w);
		}

		std::set<std::string> seen_o;
		std::vector<std::string> original;
		for (const auto & w : original_raw)
		{
			if (seen_o.insert(w).second)
				original.push_back(w);
		}

		const auto result = build_compare_words(translated, original);

		std::set<std::string> seen;
		for (const auto & w : result)
			RC_ASSERT(seen.insert(w).second);
	});
}

TEST_CASE("word_match_utils::compute_best_match, highest score", "[u][pbt]")
{
	rc::prop(
	    "returned score >= all other unmatched candidates",
	    []()
	{
		auto compare_words = *rc::gen::arbitrary<std::vector<std::string>>();
		RC_PRE(!compare_words.empty());

		auto original_words = *rc::gen::arbitrary<std::vector<std::string>>();
		auto translated_words = *rc::gen::arbitrary<std::vector<std::string>>();

		auto candidate_names = *rc::gen::arbitrary<std::vector<std::string>>();
		RC_PRE(!candidate_names.empty());

		std::vector<std::pair<size_t, std::string>> candidates;
		for (size_t i = 0; i < candidate_names.size(); ++i)
			candidates.emplace_back(i, candidate_names[i]);

		const auto matched_count = *rc::gen::inRange(size_t { 0 }, candidates.size());

		std::set<size_t> matched_set;
		for (size_t i = 0; i < matched_count; ++i)
			matched_set.insert(i);

		bool has_unmatched = false;
		for (size_t i = 0; i < candidates.size(); ++i)
		{
			if (!matched_set.count(i))
			{
				has_unmatched = true;
				break;
			}
		}

		RC_PRE(has_unmatched);

		const auto result =
		    compute_best_match(compare_words, original_words, translated_words, candidates, matched_set);

		if (result.score > 0)
		{
			RC_ASSERT(result.index < candidates.size());
			RC_ASSERT(!matched_set.count(result.index));
		}

		for (size_t ni = 0; ni < candidates.size(); ++ni)
		{
			if (matched_set.count(ni))
				continue;

			auto native_words = split_words(candidates[ni].second);
			int score = count_shared_words(compare_words, native_words);
			RC_ASSERT(result.score >= score);
		}
	});
}

TEST_CASE("word_match_utils::check_all_same_name, correctness", "[u][pbt]")
{
	rc::prop(
	    "true iff all tied candidates share the winner name",
	    []()
	{
		auto base_name = *rc::gen::arbitrary<std::string>();
		RC_PRE(!base_name.empty());

		const auto candidate_count = *rc::gen::inRange(2, 8);

		std::vector<std::pair<size_t, std::string>> candidates;
		for (int i = 0; i < candidate_count; ++i)
			candidates.emplace_back(static_cast<size_t>(i), base_name);

		const auto differ_index = *rc::gen::inRange(1, candidate_count);
		const auto use_different_name = *rc::gen::arbitrary<bool>();
		if (use_different_name)
		{
			auto different = *rc::gen::arbitrary<std::string>();
			if (different == base_name || different.empty())
				different = base_name + "x";
			candidates[differ_index].second = different;
		}

		const auto compare_words = split_words(base_name);
		std::set<size_t> matched_set;

		match_result_t result;
		result.score = count_shared_words(compare_words, split_words(base_name));
		result.name = base_name;
		result.count = 2;
		result.index = 0;

		RC_PRE(result.score > 0);

		const auto all_same = check_all_same_name(compare_words, candidates, matched_set, result);

		bool expected_all_same = true;
		for (size_t ni = 0; ni < candidates.size(); ++ni)
		{
			if (matched_set.count(ni))
				continue;

			auto native_words = split_words(candidates[ni].second);
			int score = count_shared_words(compare_words, native_words);
			if (score == result.score && candidates[ni].second != result.name)
			{
				expected_all_same = false;
				break;
			}
		}

		RC_ASSERT(all_same == expected_all_same);
	});
}
