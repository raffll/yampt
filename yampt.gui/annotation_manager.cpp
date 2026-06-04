#include "annotation_manager.hpp"
#include "../yampt/dict_reader.hpp"
#include <algorithm>

std::string annotation_manager_t::to_lower(const std::string & str)
{
	std::string result = str;
	for (auto & c : result)
		c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
	return result;
}

bool annotation_manager_t::is_alpha(char c)
{
	return isalpha(static_cast<unsigned char>(c)) != 0;
}

void annotation_manager_t::rebuild(const std::vector<dict_source_t> & sources)
{
	dial_topics_.clear();
	glossary_terms_.clear();

	for (const auto & src : sources)
	{
		if (!src.dict)
			continue;

		auto dial_it = src.dict->find(tools_t::rec_type_t::dial);
		if (dial_it != src.dict->end())
		{
			for (const auto & entry : dial_it->second.records)
			{
				if (entry.key_text.empty())
					continue;

				std::string key_lower = to_lower(entry.key_text);
				dial_topics_.push_back({ key_lower, entry.new_text, src.name });
			}
		}

		auto fnam_it = src.dict->find(tools_t::rec_type_t::fnam);
		if (fnam_it != src.dict->end())
		{
			for (const auto & entry : fnam_it->second.records)
			{
				if (entry.old_text.empty())
					continue;
				if (entry.old_text == entry.new_text)
					continue;

				std::string key_lower = to_lower(entry.old_text);
				glossary_terms_.push_back({ key_lower, entry.new_text, src.name });
			}
		}
	}

	std::sort(
	    dial_topics_.begin(),
	    dial_topics_.end(),
	    [](const topic_entry_t & a, const topic_entry_t & b) { return a.key_lower.size() > b.key_lower.size(); });

	std::sort(
	    glossary_terms_.begin(),
	    glossary_terms_.end(),
	    [](const topic_entry_t & a, const topic_entry_t & b) { return a.key_lower.size() > b.key_lower.size(); });
}

std::vector<annotation_t> annotation_manager_t::annotate(const std::string & text, tools_t::rec_type_t type) const
{
	std::vector<annotation_t> results;

	if (type != tools_t::rec_type_t::info)
		return results;

	if (text.empty())
		return results;

	std::string text_lower = to_lower(text);

	std::vector<annotation_t> hyperlinks;
	std::vector<annotation_t> glossary;

	find_matches(text_lower, text, dial_topics_, annotation_t::dial_topic, hyperlinks);
	find_matches(text_lower, text, glossary_terms_, annotation_t::glossary_term, glossary);

	results.reserve(hyperlinks.size() + glossary.size());
	for (auto & ann : hyperlinks)
		results.push_back(std::move(ann));

	for (auto & ann : glossary)
	{
		bool overlaps = false;
		for (const auto & h : hyperlinks)
		{
			if (ann.start < h.end && ann.end > h.start)
			{
				overlaps = true;
				break;
			}
		}
		if (!overlaps)
			results.push_back(std::move(ann));
	}

	return results;
}

void annotation_manager_t::find_matches(
    const std::string & text_lower,
    const std::string & text_original,
    const std::vector<topic_entry_t> & terms,
    annotation_t::kind_t kind,
    std::vector<annotation_t> & results) const
{
	for (const auto & term : terms)
	{
		if (term.key_lower.empty())
			continue;

		size_t pos = 0;
		while ((pos = text_lower.find(term.key_lower, pos)) != std::string::npos)
		{
			if (pos > 0 && is_alpha(text_lower[pos - 1]))
			{
				pos += term.key_lower.size();
				continue;
			}

			annotation_t ann;
			ann.start = pos;
			ann.end = pos + term.key_lower.size();
			ann.kind = kind;
			ann.old_text = text_original.substr(pos, term.key_lower.size());
			ann.new_text = term.new_text;
			ann.source = term.source;
			results.push_back(std::move(ann));

			pos += term.key_lower.size();
		}
	}
}

void annotation_manager_t::load_npc_flags(const std::string &)
{}

const std::string & annotation_manager_t::get_speaker_gender(const std::string & npc_id) const
{
	static const std::string empty;
	auto it = npc_flags_.find(npc_id);
	if (it == npc_flags_.end())
		return empty;
	return it->second;
}

void annotation_manager_t::load_enchantments(const std::string & path)
{
	enchantments_.clear();

	dict_reader_t reader(path);
	if (!reader.is_loaded())
		return;

	const auto & dict = reader.get_dict();
	auto it = dict.find(tools_t::rec_type_t::fnam);
	if (it == dict.end())
		return;

	const auto & chapter = it->second;
	for (const auto & entry : chapter.records)
	{
		if (entry.key_text.empty())
			continue;

		enchantments_[entry.key_text] = entry.new_text;
	}
}

const std::string & annotation_manager_t::get_enchantment(const std::string & key) const
{
	static const std::string empty;
	auto it = enchantments_.find(key);
	if (it == enchantments_.end())
		return empty;
	return it->second;
}

bool annotation_manager_t::has_enchantment(const std::string & key) const
{
	return enchantments_.count(key) > 0;
}
