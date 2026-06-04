#pragma once

#include "../yampt/tools.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct annotation_t
{
	size_t start;
	size_t end;

	enum kind_t
	{
		dial_topic,
		glossary_term,
		speaker_gender
	};

	kind_t kind;
	std::string old_text;
	std::string new_text;
	std::string source;
};

struct dict_source_t
{
	const tools_t::dict_t * dict;
	std::string name;
};

class annotation_manager_t
{
public:
	void rebuild(const std::vector<dict_source_t> & sources);
	std::vector<annotation_t> annotate(const std::string & text, tools_t::rec_type_t type) const;
	std::vector<annotation_t> annotate_translated(const std::string & text, tools_t::rec_type_t type) const;

	void load_npc_flags(const std::string & path);
	void load_enchantments(const std::string & path);

	const std::string & get_speaker_gender(const std::string & npc_id) const;
	const std::string & get_enchantment(const std::string & key) const;
	bool has_enchantment(const std::string & key) const;

private:
	struct topic_entry_t
	{
		std::string key_lower;
		std::string new_text;
		std::string source;
	};

	std::vector<topic_entry_t> dial_topics_;
	std::vector<topic_entry_t> glossary_terms_;
	std::unordered_map<std::string, std::string> npc_flags_;
	std::unordered_map<std::string, std::string> enchantments_;

	static std::string to_lower(const std::string & str);
	static bool is_alpha(char c);

	void find_matches(
	    const std::string & text_lower,
	    const std::string & text_original,
	    const std::vector<topic_entry_t> & terms,
	    annotation_t::kind_t kind,
	    std::vector<annotation_t> & results) const;
};
