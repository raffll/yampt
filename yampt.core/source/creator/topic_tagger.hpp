#pragma once

#include "utility/domain_types.hpp"
#include "utility/keyword_trie.hpp"
#include <string>

struct topic_tag_result_t
{
	std::string text;
	int tags_inserted = 0;
};

struct apply_tags_result_t
{
	int entries_changed = 0;
	int tags_inserted = 0;
};

class topic_tagger_t
{
public:
	void seed_topics(const dict_t & dict);
	topic_tag_result_t tag_line(const std::string & line) const;

	static std::string strip_tags(const std::string & line);

private:
	keyword_trie_t m_topic_trie;
};

apply_tags_result_t apply_topic_tags(dict_t & dict);
