#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

struct keyword_node_t
{
	std::string keyword;
	std::string topic_id;
	std::map<char, keyword_node_t> children;
};

struct keyword_match_t
{
	size_t start;
	size_t length;
	std::string keyword;
	std::string topic_id;
};

class keyword_trie_t
{
public:
	void seed(std::string_view keyword, std::string_view topic_id);
	void clear();
	std::vector<keyword_match_t> find_matches(std::string_view text) const;

private:
	keyword_node_t m_root;

	void build_trie(std::string_view keyword, std::string_view topic_id, size_t depth, keyword_node_t & node);
	static bool is_word_separator(char character);
	static char to_lower_char(char character);
};
