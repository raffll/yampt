#include "glossary.hpp"
#include <io/dict_reader.hpp>
#include <utility/string_utils.hpp>
#include <algorithm>
#include <set>

bool glossary_t::is_alpha(char c)
{
	return isalpha(static_cast<unsigned char>(c)) != 0;
}

bool glossary_t::is_trusted_status(status_t status)
{
	return status == status_t::translated || status == status_t::reused || status == status_t::adapted;
}

void glossary_t::collect_dial_entries(const dict_source_t & source)
{
	auto dial_it = source.dict->find(tools_t::rec_type_t::dial);
	if (dial_it == source.dict->end())
		return;

	for (const auto & entry : dial_it->second.records)
	{
		if (entry.old_text.empty())
			continue;

		dial_topics_.push_back({ string_utils::to_lower(entry.old_text), entry.new_text, source.name });
	}
}

void glossary_t::collect_glossary_entries(const dict_source_t & source, tools_t::rec_type_t record_type)
{
	auto chapter_it = source.dict->find(record_type);
	if (chapter_it == source.dict->end())
		return;

	for (const auto & entry : chapter_it->second.records)
	{
		if (entry.old_text.empty())
			continue;

		if (entry.old_text == entry.new_text)
			continue;

		if (!is_trusted_status(entry.status))
			continue;

		glossary_terms_.push_back({ string_utils::to_lower(entry.old_text), entry.new_text, source.name });
	}
}

void glossary_t::sort_by_length_descending(std::vector<topic_entry_t> & entries)
{
	std::sort(
	    entries.begin(),
	    entries.end(),
	    [](const topic_entry_t & left, const topic_entry_t & right)
	{ return left.key_lower.size() > right.key_lower.size(); });
}

void glossary_t::rebuild(const std::vector<dict_source_t> & sources)
{
	dial_topics_.clear();
	glossary_terms_.clear();

	for (const auto & source : sources)
	{
		if (!source.dict)
			continue;

		collect_dial_entries(source);
		collect_glossary_entries(source, tools_t::rec_type_t::fnam);
		collect_glossary_entries(source, tools_t::rec_type_t::cell);
		collect_glossary_entries(source, tools_t::rec_type_t::rnam);
		collect_glossary_entries(source, tools_t::rec_type_t::indx);
	}

	sort_by_length_descending(dial_topics_);
	sort_by_length_descending(glossary_terms_);
}

void glossary_t::update_term(tools_t::rec_type_t type, const std::string & old_text, const std::string & new_text)
{
	if (type == tools_t::rec_type_t::dial)
	{
		update_vector(dial_topics_, old_text, new_text);
	}
	else if (type == tools_t::rec_type_t::fnam || type == tools_t::rec_type_t::cell)
	{
		if (old_text == new_text || new_text.empty())
			remove_from_vector(glossary_terms_, old_text);
		else
			update_vector(glossary_terms_, old_text, new_text);
	}
}

void glossary_t::update_vector(
    std::vector<topic_entry_t> & vec,
    const std::string & old_text,
    const std::string & new_text)
{
	const auto & key_lower = string_utils::to_lower(old_text);

	for (auto & entry : vec)
	{
		if (entry.key_lower == key_lower)
		{
			entry.new_text = new_text;
			return;
		}
	}

	if (new_text.empty())
		return;

	topic_entry_t new_entry { key_lower, new_text, {} };
	auto insert_pos = std::lower_bound(
	    vec.begin(),
	    vec.end(),
	    new_entry,
	    [](const topic_entry_t & left, const topic_entry_t & right)
	{ return left.key_lower.size() > right.key_lower.size(); });
	vec.insert(insert_pos, std::move(new_entry));
}

void glossary_t::remove_from_vector(std::vector<topic_entry_t> & vec, const std::string & old_text)
{
	const auto & key_lower = string_utils::to_lower(old_text);
	vec.erase(
	    std::remove_if(
	        vec.begin(), vec.end(), [&key_lower](const topic_entry_t & entry) { return entry.key_lower == key_lower; }),
	    vec.end());
}

std::vector<annotation_t> glossary_t::annotate(const std::string & text, tools_t::rec_type_t type) const
{
	(void)type;
	std::vector<annotation_t> results;

	if (text.empty())
		return results;

	const auto & text_lower = string_utils::to_lower(text);

	std::vector<annotation_t> hyperlinks;
	std::vector<annotation_t> glossary;

	find_matches(text_lower, text, dial_topics_, annotation_t::dial_topic, hyperlinks);
	find_matches(text_lower, text, glossary_terms_, annotation_t::glossary_term, glossary);

	results.reserve(hyperlinks.size() + glossary.size());
	for (auto & annotation : hyperlinks)
		results.push_back(std::move(annotation));

	for (auto & annotation : glossary)
	{
		bool overlaps = false;
		for (const auto & hyperlink : hyperlinks)
		{
			if (annotation.start < hyperlink.end && annotation.end > hyperlink.start)
			{
				overlaps = true;
				break;
			}
		}
		if (!overlaps)
			results.push_back(std::move(annotation));
	}

	return results;
}

std::vector<annotation_t> glossary_t::annotate_translated(const std::string & text, tools_t::rec_type_t type) const
{
	(void)type;
	std::vector<annotation_t> results;

	if (text.empty())
		return results;

	const auto & text_lower = string_utils::to_lower(text);

	std::set<std::string> seen_topics;

	for (const auto & topic : dial_topics_)
	{
		if (topic.new_text.empty())
			continue;

		const auto & topic_lower = string_utils::to_lower(topic.new_text);
		if (!seen_topics.insert(topic_lower).second)
			continue;

		size_t search_pos = 0;
		while ((search_pos = text_lower.find(topic_lower, search_pos)) != std::string::npos)
		{
			if (search_pos > 0 && is_alpha(text_lower[search_pos - 1]))
			{
				search_pos += topic_lower.size();
				continue;
			}

			annotation_t annotation;
			annotation.start = search_pos;
			annotation.end = search_pos + topic_lower.size();
			annotation.kind = annotation_t::dial_topic;
			annotation.old_text = text.substr(search_pos, topic_lower.size());
			annotation.new_text = topic.new_text;
			annotation.source = topic.source;
			results.push_back(std::move(annotation));

			search_pos += topic_lower.size();
		}
	}

	return results;
}

void glossary_t::find_matches(
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

		size_t search_pos = 0;
		while ((search_pos = text_lower.find(term.key_lower, search_pos)) != std::string::npos)
		{
			if (search_pos > 0 && is_alpha(text_lower[search_pos - 1]))
			{
				search_pos += term.key_lower.size();
				continue;
			}

			annotation_t annotation;
			annotation.start = search_pos;
			annotation.end = search_pos + term.key_lower.size();
			annotation.kind = kind;
			annotation.old_text = text_original.substr(search_pos, term.key_lower.size());
			annotation.new_text = term.new_text;
			annotation.source = term.source;
			results.push_back(std::move(annotation));

			search_pos += term.key_lower.size();
		}
	}
}

void glossary_t::load_npc_flags(const std::string &)
{}

const std::string & glossary_t::get_speaker_gender(const std::string & npc_id) const
{
	static const std::string empty;
	auto it_flag = npc_flags_.find(npc_id);
	if (it_flag == npc_flags_.end())
		return empty;
	return it_flag->second;
}

void glossary_t::load_enchantments(const std::string & path)
{
	enchantments_.clear();

	dict_reader_t reader(path);
	if (!reader.is_loaded())
		return;

	const auto & loaded_dict = reader.get_dict();
	auto chapter_it = loaded_dict.find(tools_t::rec_type_t::fnam);
	if (chapter_it == loaded_dict.end())
		return;

	const auto & chapter = chapter_it->second;
	for (const auto & entry : chapter.records)
	{
		if (entry.key_text.empty())
			continue;

		enchantments_[entry.key_text] = entry.new_text;
	}
}

const std::string & glossary_t::get_enchantment(const std::string & key_text) const
{
	static const std::string empty;
	auto it_ench = enchantments_.find(key_text);
	if (it_ench == enchantments_.end())
		return empty;
	return it_ench->second;
}

bool glossary_t::has_enchantment(const std::string & key_text) const
{
	return enchantments_.count(key_text) > 0;
}

std::vector<glossary_t::glossary_match_t> glossary_t::find_glossary_matches(const std::string & source_text) const
{
	std::vector<glossary_match_t> matches;
	const auto & text_lower = string_utils::to_lower(source_text);
	std::vector<bool> covered(source_text.size(), false);

	for (const auto & term : glossary_terms_)
	{
		if (term.key_lower.empty() || term.new_text.empty())
			continue;

		size_t search_pos = 0;
		while ((search_pos = text_lower.find(term.key_lower, search_pos)) != std::string::npos)
		{
			bool has_overlap = false;
			for (size_t index = search_pos; index < search_pos + term.key_lower.size(); ++index)
			{
				if (covered[index])
				{
					has_overlap = true;
					break;
				}
			}

			if (has_overlap)
			{
				search_pos += term.key_lower.size();
				continue;
			}

			if (search_pos > 0 && is_alpha(text_lower[search_pos - 1]))
			{
				search_pos += term.key_lower.size();
				continue;
			}

			const auto end_pos = search_pos + term.key_lower.size();
			if (end_pos < text_lower.size() && is_alpha(text_lower[end_pos]))
			{
				search_pos += term.key_lower.size();
				continue;
			}

			for (size_t index = search_pos; index < end_pos; ++index)
				covered[index] = true;

			matches.push_back({ search_pos, term.key_lower.size(), term.new_text });
			search_pos = end_pos;
		}
	}

	return matches;
}

std::string glossary_t::apply_glossary(const std::string & translated_text) const
{
	auto text_lower = string_utils::to_lower(translated_text);
	std::string result = translated_text;

	for (const auto & term : glossary_terms_)
	{
		if (term.key_lower.empty() || term.new_text.empty())
			continue;

		size_t search_pos = 0;
		while ((search_pos = text_lower.find(term.key_lower, search_pos)) != std::string::npos)
		{
			if (search_pos > 0 && is_alpha(text_lower[search_pos - 1]))
			{
				search_pos += term.key_lower.size();
				continue;
			}

			const auto end_pos = search_pos + term.key_lower.size();
			if (end_pos < text_lower.size() && is_alpha(text_lower[end_pos]))
			{
				search_pos += term.key_lower.size();
				continue;
			}

			result.replace(search_pos, term.key_lower.size(), term.new_text);
			text_lower = string_utils::to_lower(result);
			search_pos += term.new_text.size();
		}
	}

	return result;
}
