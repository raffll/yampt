#include <catch2/catch_all.hpp>
#include <rapidcheck/catch.h>
#include <utility/keyword_trie.hpp>
#include <algorithm>
#include <cctype>
#include <map>
#include <rapidcheck.h>
#include <string>
#include <vector>

namespace {

struct reference_match_t
{
	size_t start;
	size_t length;
};

bool ref_is_word_separator(char character)
{
	constexpr std::string_view separators = "\n\r \t'\"([";
	return separators.find(character) != std::string_view::npos;
}

char ref_to_lower(char character)
{
	return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
}

bool ref_ci_starts_with(std::string_view text, std::string_view prefix)
{
	if (text.size() < prefix.size())
		return false;

	for (size_t index = 0; index < prefix.size(); ++index)
	{
		if (ref_to_lower(text[index]) != ref_to_lower(prefix[index]))
			return false;
	}

	return true;
}

struct ref_entry_t
{
	std::string keyword;
	std::string topic_id;
	std::map<char, ref_entry_t> children;
};

void ref_build_trie(std::string_view keyword, std::string_view topic_id, size_t depth, ref_entry_t & entry)
{
	const char lower_char = ref_to_lower(keyword[depth]);
	auto it_found = entry.children.find(lower_char);

	if (it_found == entry.children.end())
	{
		entry.children[lower_char].topic_id = std::string(topic_id);
		entry.children[lower_char].keyword = std::string(keyword);
		return;
	}

	auto & existing = it_found->second;

	if (!existing.keyword.empty())
	{
		const auto & existing_keyword = existing.keyword;
		if (existing_keyword.size() == keyword.size())
		{
			bool is_equal = true;
			for (size_t index = 0; index < keyword.size(); ++index)
			{
				if (ref_to_lower(keyword[index]) != ref_to_lower(existing_keyword[index]))
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
			ref_build_trie(existing_keyword, existing.topic_id, depth + 1, existing);
			existing.keyword.clear();
			existing.topic_id.clear();
		}
	}

	if (depth + 1 == keyword.size())
	{
		existing.topic_id = std::string(topic_id);
		existing.keyword = std::string(keyword);
	}
	else
	{
		ref_build_trie(keyword, topic_id, depth + 1, existing);
	}
}

std::vector<reference_match_t> ref_highlight_keywords(std::string_view text, const ref_entry_t & root)
{
	std::vector<reference_match_t> candidates;

	for (size_t position = 0; position < text.size(); ++position)
	{
		if (position != 0)
		{
			if (!ref_is_word_separator(text[position - 1]))
				continue;
		}

		const ref_entry_t * current = &root;
		for (size_t walk = position; walk < text.size(); ++walk)
		{
			auto it_child = current->children.find(ref_to_lower(text[walk]));
			if (it_child == current->children.end())
				break;

			current = &it_child->second;

			if (!current->keyword.empty())
			{
				const auto matched_so_far = walk + 1 - position;
				if (matched_so_far >= current->keyword.size())
				{
					candidates.push_back({ position, current->keyword.size() });
					continue;
				}

				const auto remaining_text = text.substr(walk + 1);
				const auto remaining_keyword = std::string_view(current->keyword).substr(matched_so_far);
				if (ref_ci_starts_with(remaining_text, remaining_keyword))
				{
					candidates.push_back({ position, current->keyword.size() });
				}
			}
		}
	}

	std::vector<reference_match_t> output;

	while (!candidates.empty())
	{
		size_t longest_size = 0;
		size_t longest_index = 0;

		for (size_t index = 0; index < candidates.size(); ++index)
		{
			if (candidates[index].length > longest_size)
			{
				longest_size = candidates[index].length;
				longest_index = index;
			}

			const auto next_index = index + 1;
			if (next_index == candidates.size())
				break;

			const auto current_end = candidates[index].start + candidates[index].length;
			if (current_end <= candidates[next_index].start)
				break;
		}

		auto chosen = candidates[longest_index];
		candidates.erase(candidates.begin() + static_cast<ptrdiff_t>(longest_index));
		output.push_back(chosen);

		const auto chosen_start = chosen.start;
		const auto chosen_end = chosen_start + chosen.length;

		std::erase_if(
		    candidates,
		    [chosen_start, chosen_end](const reference_match_t & match)
		{ return match.start < chosen_end && (match.start + match.length) > chosen_start; });
	}

	std::sort(
	    output.begin(),
	    output.end(),
	    [](const reference_match_t & left, const reference_match_t & right) { return left.start < right.start; });

	return output;
}

rc::Gen<std::string> gen_topic_word()
{
	return rc::gen::exec([]()
	{
		const auto length = *rc::gen::inRange(1, 12);
		std::string result;
		result.reserve(length);
		for (int index = 0; index < length; ++index)
			result += *rc::gen::inRange('a', 'z');
		return result;
	});
}

rc::Gen<std::string> gen_topic_name()
{
	return rc::gen::exec([]()
	{
		const auto word_count = *rc::gen::inRange(1, 4);
		std::string result;

		for (int index = 0; index < word_count; ++index)
		{
			if (!result.empty())
				result += ' ';
			result += *gen_topic_word();
		}

		return result;
	});
}

rc::Gen<char> gen_word_separator()
{
	return rc::gen::element(' ', '\t', '\'', '"', '(', '[', '\n', '\r');
}

rc::Gen<std::string> gen_dialogue_text(const std::vector<std::string> & topics)
{
	return rc::gen::exec([&topics]()
	{
		std::string result;
		const auto segment_count = *rc::gen::inRange(1, 8);

		for (int index = 0; index < segment_count; ++index)
		{
			if (!result.empty())
			{
				const auto separator = *gen_word_separator();
				result += separator;
			}

			const auto use_topic = *rc::gen::inRange(0, 3);
			if (use_topic == 0 && !topics.empty())
			{
				const auto topic_index = *rc::gen::inRange(static_cast<size_t>(0), topics.size());
				const auto & topic = topics[topic_index];

				const auto capitalize = *rc::gen::arbitrary<bool>();
				if (capitalize && !topic.empty())
				{
					std::string upper_topic = topic;
					upper_topic[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(upper_topic[0])));
					result += upper_topic;
				}
				else
				{
					result += topic;
				}
			}
			else
			{
				const auto filler = *gen_topic_word();
				result += filler;
			}
		}

		return result;
	});
}

} // namespace

TEST_CASE("keyword_trie_t::find_matches, OpenMW-accurate matching property", "[u]")
{
	rc::prop(
	    "trie finds longest matches at word boundaries identical to reference",
	    []()
	{
		const auto topic_count = *rc::gen::inRange(1, 10);
		std::vector<std::string> topics;
		topics.reserve(topic_count);

		for (int index = 0; index < topic_count; ++index)
			topics.push_back(*gen_topic_name());

		keyword_trie_t trie;
		ref_entry_t ref_root;

		for (size_t index = 0; index < topics.size(); ++index)
		{
			const auto & topic = topics[index];
			const auto topic_id = std::to_string(index);
			trie.seed(topic, topic_id);
			ref_build_trie(topic, topic_id, 0, ref_root);
		}

		const auto text = *gen_dialogue_text(topics);
		if (text.empty())
			return;

		const auto trie_results = trie.find_matches(text);
		const auto ref_results = ref_highlight_keywords(text, ref_root);

		RC_ASSERT(trie_results.size() == ref_results.size());

		for (size_t index = 0; index < trie_results.size(); ++index)
		{
			RC_ASSERT(trie_results[index].start == ref_results[index].start);
			RC_ASSERT(trie_results[index].length == ref_results[index].length);
		}
	});
}

TEST_CASE("keyword_trie_t::find_matches, longest match wins over shorter", "[u]")
{
	keyword_trie_t trie;
	trie.seed("kwama forager", "topic_long");
	trie.seed("kwama", "topic_short");

	const auto results = trie.find_matches("the kwama forager attacks");

	REQUIRE(results.size() == 1);
	REQUIRE(results[0].start == 4);
	REQUIRE(results[0].length == 13);
	REQUIRE(results[0].keyword == "kwama forager");
}

TEST_CASE("keyword_trie_t::find_matches, word boundary required", "[u]")
{
	keyword_trie_t trie;
	trie.seed("alma", "topic_alma");

	SECTION("preceded by letter rejects")
	{
		const auto results = trie.find_matches("Balmora is great");
		REQUIRE(results.empty());
	}

	SECTION("preceded by separator accepts")
	{
		const auto results = trie.find_matches("the alma mater");
		REQUIRE(results.size() == 1);
		REQUIRE(results[0].start == 4);
		REQUIRE(results[0].length == 4);
	}

	SECTION("at start of text accepts")
	{
		const auto results = trie.find_matches("alma mater here");
		REQUIRE(results.size() == 1);
		REQUIRE(results[0].start == 0);
		REQUIRE(results[0].length == 4);
	}
}

TEST_CASE("keyword_trie_t::find_matches, case insensitive", "[u]")
{
	keyword_trie_t trie;
	trie.seed("Balmora", "topic_balmora");

	const auto results = trie.find_matches("visit BALMORA today");

	REQUIRE(results.size() == 1);
	REQUIRE(results[0].start == 6);
	REQUIRE(results[0].length == 7);
}

TEST_CASE("keyword_trie_t::find_matches, non-overlapping multiple matches", "[u]")
{
	keyword_trie_t trie;
	trie.seed("silt strider", "topic_silt");
	trie.seed("guild guide", "topic_guild");

	const auto results = trie.find_matches("use silt strider or guild guide");

	REQUIRE(results.size() == 2);
	REQUIRE(results[0].start == 4);
	REQUIRE(results[0].length == 12);
	REQUIRE(results[1].start == 20);
	REQUIRE(results[1].length == 11);
}
