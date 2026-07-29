#include "word_match_utils.hpp"

std::vector<std::string> split_words(const std::string & text)
{
	std::vector<std::string> words;
	std::string word;
	for (char c : text)
	{
		if (std::isalnum(static_cast<unsigned char>(c)))
			word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		else
		{
			if (!word.empty())
			{
				words.push_back(word);
				word.clear();
			}
		}
	}
	if (!word.empty())
		words.push_back(word);
	return words;
}

int count_shared_words(const std::vector<std::string> & source, const std::vector<std::string> & target)
{
	int count = 0;
	for (const auto & word : source)
	{
		for (const auto & target_word : target)
		{
			if (word == target_word)
			{
				count++;
				break;
			}
		}
	}
	return count;
}

std::vector<std::string> build_compare_words(
    const std::vector<std::string> & translated_words,
    const std::vector<std::string> & original_words)
{
	std::vector<std::string> compare_words = translated_words;
	for (const auto & word : original_words)
	{
		bool found = false;
		for (const auto & existing : compare_words)
		{
			if (existing == word)
			{
				found = true;
				break;
			}
		}
		if (!found)
			compare_words.push_back(word);
	}
	return compare_words;
}

match_result_t compute_best_match(
    const std::vector<std::string> & compare_words,
    const std::vector<std::string> & original_words,
    const std::vector<std::string> & translated_words,
    const std::vector<std::pair<size_t, std::string>> & candidates,
    const std::set<size_t> & matched_set)
{
	match_result_t result { 0, 0, 0, 0, 0, {} };

	for (size_t ni = 0; ni < candidates.size(); ++ni)
	{
		if (matched_set.count(ni))
			continue;

		auto native_words = split_words(candidates[ni].second);
		int score_orig = count_shared_words(original_words, native_words);
		int score_model = count_shared_words(translated_words, native_words);
		int score = count_shared_words(compare_words, native_words);

		if (score > result.score)
		{
			result.score = score;
			result.score_orig = score_orig;
			result.score_model = score_model;
			result.count = 1;
			result.index = ni;
			result.name = candidates[ni].second;
		}
		else if (score == result.score && score > 0)
		{
			result.count++;
		}
	}

	return result;
}

bool check_all_same_name(
    const std::vector<std::string> & compare_words,
    const std::vector<std::pair<size_t, std::string>> & candidates,
    const std::set<size_t> & matched_set,
    const match_result_t & result)
{
	for (size_t ni = 0; ni < candidates.size(); ++ni)
	{
		if (matched_set.count(ni))
			continue;

		auto native_words = split_words(candidates[ni].second);
		int score = count_shared_words(compare_words, native_words);
		if (score == result.score && candidates[ni].second != result.name)
			return false;
	}

	return true;
}
