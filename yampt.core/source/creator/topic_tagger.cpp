#include "creator/topic_tagger.hpp"

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

topic_tag_result_t topic_tagger_t::tag_line(const std::string & line) const
{
	const auto stripped = strip_tags(line);
	const auto matches = m_topic_trie.find_matches(stripped);

	std::string result;
	result.reserve(stripped.size() + matches.size() * 2);

	std::size_t copy_pos = 0;
	int accepted_count = 0;
	std::size_t last_span_end = 0;

	for (const auto & match : matches)
	{
		if (accepted_count != 0 && match.start < last_span_end)
			continue;

		const auto match_end = match.start + match.length;
		result.append(stripped, copy_pos, match.start - copy_pos);
		result.push_back(link_open);
		result.append(stripped, match.start, match.length);
		result.push_back(link_close);

		copy_pos = match_end;
		last_span_end = match_end;
		++accepted_count;
	}

	result.append(stripped, copy_pos, std::string::npos);

	return { result, accepted_count };
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
