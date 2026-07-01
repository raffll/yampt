#include "keyword_trie.hpp"
#include <algorithm>
#include <cctype>

char keyword_trie_t::to_lower_char(char character)
{
	return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
}

bool keyword_trie_t::is_word_separator(char character)
{
	constexpr std::string_view separators = "\n\r \t'\"([";
	return separators.find(character) != std::string_view::npos;
}

void keyword_trie_t::seed(std::string_view keyword, std::string_view topic_id)
{
	if (keyword.empty())
		return;

	build_trie(keyword, topic_id, 0, m_root);
}

void keyword_trie_t::clear()
{
	m_root.children.clear();
	m_root.keyword.clear();
	m_root.topic_id.clear();
}

void keyword_trie_t::build_trie(
    std::string_view keyword,
    std::string_view topic_id,
    size_t depth,
    keyword_node_t & node)
{
	const char lower_char = to_lower_char(keyword[depth]);
	auto it_found = node.children.find(lower_char);

	if (it_found == node.children.end())
	{
		node.children[lower_char].topic_id = std::string(topic_id);
		node.children[lower_char].keyword = std::string(keyword);
		return;
	}

	auto & existing_node = it_found->second;

	if (!existing_node.keyword.empty())
	{
		const auto & existing_keyword = existing_node.keyword;
		if (existing_keyword.size() == keyword.size())
		{
			bool is_equal = true;
			for (size_t index = 0; index < keyword.size(); ++index)
			{
				if (to_lower_char(keyword[index]) != to_lower_char(existing_keyword[index]))
				{
					is_equal = false;
					break;
				}
			}

			if (is_equal)
				return;
		}

		if (depth + 1 < existing_keyword.size())
		{
			build_trie(existing_keyword, existing_node.topic_id, depth + 1, existing_node);
			existing_node.keyword.clear();
			existing_node.topic_id.clear();
		}
	}

	if (depth + 1 == keyword.size())
	{
		existing_node.topic_id = std::string(topic_id);
		existing_node.keyword = std::string(keyword);
	}
	else
	{
		build_trie(keyword, topic_id, depth + 1, existing_node);
	}
}

static bool ci_starts_with(std::string_view text, std::string_view prefix)
{
	if (text.size() < prefix.size())
		return false;

	for (size_t index = 0; index < prefix.size(); ++index)
	{
		const auto left = static_cast<char>(std::tolower(static_cast<unsigned char>(text[index])));
		const auto right = static_cast<char>(std::tolower(static_cast<unsigned char>(prefix[index])));
		if (left != right)
			return false;
	}

	return true;
}

static void resolve_overlaps(std::vector<keyword_match_t> & matches, std::vector<keyword_match_t> & output)
{
	while (!matches.empty())
	{
		size_t longest_size = 0;
		size_t longest_index = 0;

		for (size_t index = 0; index < matches.size(); ++index)
		{
			if (matches[index].length > longest_size)
			{
				longest_size = matches[index].length;
				longest_index = index;
			}

			const auto next_index = index + 1;
			if (next_index == matches.size())
				break;

			const auto current_end = matches[index].start + matches[index].length;
			if (current_end <= matches[next_index].start)
				break;
		}

		auto chosen = matches[longest_index];
		matches.erase(matches.begin() + static_cast<ptrdiff_t>(longest_index));
		output.push_back(std::move(chosen));

		const auto & last_added = output.back();
		const auto chosen_start = last_added.start;
		const auto chosen_end = chosen_start + last_added.length;

		std::erase_if(matches, [chosen_start, chosen_end](const keyword_match_t & match)
		{ return match.start < chosen_end && (match.start + match.length) > chosen_start; });
	}

	std::sort(output.begin(), output.end(), [](const keyword_match_t & left, const keyword_match_t & right)
	{ return left.start < right.start; });
}

std::vector<keyword_match_t> keyword_trie_t::find_matches(std::string_view text) const
{
	std::vector<keyword_match_t> candidates;

	for (size_t position = 0; position < text.size(); ++position)
	{
		if (position != 0)
		{
			if (!is_word_separator(text[position - 1]))
				continue;
		}

		const keyword_node_t * current = &m_root;
		for (size_t walk = position; walk < text.size(); ++walk)
		{
			auto it_child = current->children.find(to_lower_char(text[walk]));
			if (it_child == current->children.end())
				break;

			current = &it_child->second;

			if (current->keyword.empty())
				continue;

			const auto matched_so_far = walk + 1 - position;
			if (matched_so_far >= current->keyword.size())
			{
				candidates.push_back({ position, current->keyword.size(), current->keyword, current->topic_id });
				continue;
			}

			const auto remaining_text = text.substr(walk + 1);
			const auto remaining_keyword = std::string_view(current->keyword).substr(matched_so_far);
			if (ci_starts_with(remaining_text, remaining_keyword))
			{
				candidates.push_back({ position, current->keyword.size(), current->keyword, current->topic_id });
			}
		}
	}

	std::vector<keyword_match_t> output;
	resolve_overlaps(candidates, output);
	return output;
}
