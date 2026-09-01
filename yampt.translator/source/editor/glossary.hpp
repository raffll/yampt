#pragma once

#include <utility/domain_types.hpp>
#include <utility/keyword_trie.hpp>
#include <string>
#include <vector>

struct annotation_t
{
	size_t start;
	size_t end;

	enum kind_t
	{
		dial_topic,
		glossary_term,
		speaker_gender,
		inflection_form
	};

	kind_t kind;
	std::string old_text;
	std::string new_text;
	std::string source;
};

struct dict_source_t
{
	const dict_t * dict;
	std::string name;
};

class glossary_t
{
public:
	void rebuild(const std::vector<dict_source_t> & sources);
	void update_term(rec_type_t type, const std::string & old_text, const std::string & new_text);
	std::vector<annotation_t> annotate(const std::string & text) const;
	std::vector<annotation_t> annotate_translated(const std::string & text) const;

	std::string apply_glossary(const std::string & translated_text) const;

	struct glossary_match_t
	{
		size_t start;
		size_t length;
		std::string replacement;
	};

	std::vector<glossary_match_t> find_glossary_matches(const std::string & source_text) const;

private:
	struct topic_entry_t
	{
		std::string key_lower;
		std::string new_text;
		std::string source;
	};

	keyword_trie_t m_dial_trie;

	std::vector<topic_entry_t> m_dial_topics;
	std::vector<topic_entry_t> m_glossary_terms;

	static bool is_alpha(char c);
	static bool is_trusted_status(status_t status);

	void collect_dial_entries(const dict_source_t & source);
	void collect_glossary_entries(const dict_source_t & source, rec_type_t record_type);
	void sort_by_length_descending(std::vector<topic_entry_t> & entries);
	void rebuild_dial_trie();

	void update_vector(std::vector<topic_entry_t> & vec, const std::string & old_text, const std::string & new_text);
	void remove_from_vector(std::vector<topic_entry_t> & vec, const std::string & old_text);

	void find_at_prefix_hyperlinks(const std::string & text, std::vector<annotation_t> & results) const;

	void find_matches_trie(const std::string & text_original, std::vector<annotation_t> & results) const;

	void find_matches_legacy(
	    const std::string & text_lower,
	    const std::string & text_original,
	    const std::vector<topic_entry_t> & terms,
	    annotation_t::kind_t kind,
	    std::vector<annotation_t> & results) const;
};
