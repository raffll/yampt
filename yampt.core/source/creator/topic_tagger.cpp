#include "creator/topic_tagger.hpp"

#include <algorithm>

namespace
{
constexpr char link_open = '@';
constexpr char link_close = '#';
constexpr std::size_t minimum_tag_length = 3;

bool is_whitespace_only(const std::string & text)
{
	return text.empty() || text.find_first_not_of(" \t\r\n") == std::string::npos;
}

bool has_control_characters(const std::string & text)
{
	for (const auto character : text)
	{
		const auto value = static_cast<unsigned char>(character);
		if (value < 0x20 && value != '\t' && value != '\r' && value != '\n')
			return true;
	}

	return false;
}

bool is_voice_info(const record_entry_t & entry)
{
	return entry.key_text.rfind("V^", 0) == 0;
}

bool should_skip_record(const record_entry_t & entry)
{
	if (entry.status != status_t::translated)
		return true;

	if (is_whitespace_only(entry.new_text))
		return true;

	if (entry.new_text.size() < minimum_tag_length)
		return true;

	if (has_control_characters(entry.old_text))
		return true;

	if (has_control_characters(entry.new_text))
		return true;

	return false;
}
}

void topic_tagger_t::seed_topics(const dict_t & dict)
{
	m_topic_trie.clear();

	const auto it_dial = dict.find(rec_type_t::dial);
	if (it_dial == dict.end())
		return;

	for (const auto & record : it_dial->second.records)
	{
		const auto & standard_form = record.new_text;
		if (standard_form.empty())
			continue;

		m_topic_trie.seed(standard_form, standard_form);
	}
}

void topic_tagger_t::seed_inflections(const std::vector<std::pair<std::string, std::string>> & inflected_forms)
{
	m_inflection_trie.clear();

	for (const auto & [inflected_form, standard_form] : inflected_forms)
	{
		if (inflected_form.empty())
			continue;

		m_inflection_trie.seed(inflected_form, standard_form);
	}
}

namespace
{
struct accepted_span_t
{
	std::size_t start;
	std::size_t end;
};

bool overlaps_any(const std::vector<accepted_span_t> & spans, std::size_t start, std::size_t end)
{
	for (const auto & span : spans)
	{
		if (start < span.end && end > span.start)
			return true;
	}

	return false;
}

void collect_matches(
    const std::vector<keyword_match_t> & matches,
    std::vector<accepted_span_t> & accepted)
{
	for (const auto & match : matches)
	{
		const auto start = match.start;
		const auto end = match.start + match.length;

		if (overlaps_any(accepted, start, end))
			continue;

		accepted.push_back({ start, end });
	}
}
}

topic_tag_result_t topic_tagger_t::tag_line(const std::string & line) const
{
	const auto stripped = strip_tags(line);

	std::vector<accepted_span_t> accepted;
	collect_matches(m_topic_trie.find_matches(stripped), accepted);
	collect_matches(m_inflection_trie.find_matches(stripped), accepted);

	std::sort(
	    accepted.begin(),
	    accepted.end(),
	    [](const accepted_span_t & first, const accepted_span_t & second) { return first.start < second.start; });

	std::string result;
	result.reserve(stripped.size() + accepted.size() * 2);

	std::size_t copy_pos = 0;
	for (const auto & span : accepted)
	{
		result.append(stripped, copy_pos, span.start - copy_pos);
		result.push_back(link_open);
		result.append(stripped, span.start, span.end - span.start);
		result.push_back(link_close);
		copy_pos = span.end;
	}

	result.append(stripped, copy_pos, std::string::npos);

	return { result, static_cast<int>(accepted.size()) };
}

std::string topic_tagger_t::strip_tags(const std::string & line)
{
	std::string result;
	result.reserve(line.size());

	std::size_t scan_pos = 0;
	while (scan_pos < line.size())
	{
		const auto open_pos = line.find(link_open, scan_pos);
		if (open_pos == std::string::npos)
		{
			result.append(line, scan_pos, std::string::npos);
			break;
		}

		const auto close_pos = line.find(link_close, open_pos + 1);
		if (close_pos == std::string::npos)
		{
			result.append(line, scan_pos, std::string::npos);
			break;
		}

		result.append(line, scan_pos, open_pos - scan_pos);
		result.append(line, open_pos + 1, close_pos - (open_pos + 1));
		scan_pos = close_pos + 1;
	}

	return result;
}

apply_tags_result_t apply_topic_tags(dict_t & dict)
{
	topic_tagger_t tagger;
	tagger.seed_topics(dict);

	apply_tags_result_t result;

	const auto it_info = dict.find(rec_type_t::info);
	if (it_info == dict.end())
		return result;

	for (auto & entry : it_info->second.records)
	{
		if (is_voice_info(entry))
			continue;

		if (should_skip_record(entry))
			continue;

		const auto tagged = tagger.tag_line(entry.new_text);
		if (tagged.text == entry.new_text)
			continue;

		entry.new_text = tagged.text;
		++result.entries_changed;
		result.tags_inserted += tagged.tags_inserted;
	}

	return result;
}
