#include "phrase_form_builder.hpp"

#include <unordered_set>

namespace phrase_form_builder {

namespace {

constexpr char word_separator = ' ';

std::string join_words(const std::vector<std::string> & words)
{
	std::string result;
	for (size_t index = 0; index < words.size(); ++index)
	{
		if (index > 0)
			result += word_separator;

		result += words[index];
	}

	return result;
}

bool advance_indices(
    std::vector<size_t> & indices,
    const std::vector<std::vector<std::string>> & per_word_candidate_forms)
{
	for (size_t position = indices.size(); position-- > 0;)
	{
		++indices[position];
		if (indices[position] < per_word_candidate_forms[position].size())
			return true;

		indices[position] = 0;
	}

	return false;
}

std::vector<std::string> select_words(
    const std::vector<size_t> & indices,
    const std::vector<std::vector<std::string>> & per_word_candidate_forms)
{
	std::vector<std::string> words;
	words.reserve(indices.size());
	for (size_t position = 0; position < indices.size(); ++position)
		words.push_back(per_word_candidate_forms[position][indices[position]]);

	return words;
}

bool has_empty_position(const std::vector<std::vector<std::string>> & per_word_candidate_forms)
{
	for (const auto & candidates : per_word_candidate_forms)
	{
		if (candidates.empty())
			return true;
	}

	return false;
}

} // namespace

std::vector<std::string> build_phrase_forms(
    const std::vector<std::vector<std::string>> & per_word_candidate_forms,
    const phrase_validity_predicate_t & is_valid_phrase,
    int max_phrase_forms)
{
	if (per_word_candidate_forms.empty() || max_phrase_forms <= 0)
		return {};

	if (has_empty_position(per_word_candidate_forms))
		return {};

	std::vector<std::string> results;
	std::unordered_set<std::string> seen;
	std::vector<size_t> indices(per_word_candidate_forms.size(), 0);

	do
	{
		const auto words = select_words(indices, per_word_candidate_forms);
		if (!is_valid_phrase(words))
			continue;

		auto joined = join_words(words);
		if (!seen.insert(joined).second)
			continue;

		results.push_back(std::move(joined));
		if (static_cast<int>(results.size()) >= max_phrase_forms)
			return results;
	} while (advance_indices(indices, per_word_candidate_forms));

	return results;
}

} // namespace phrase_form_builder
