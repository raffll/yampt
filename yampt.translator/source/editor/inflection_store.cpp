#include "inflection_store.hpp"

#include <io/loc_file_reader.hpp>
#include <utility/string_utils.hpp>
#include <set>

void inflection_store_t::rebuild(const std::vector<std::string> & loc_paths, codepage_t codepage)
{
	m_entries.clear();

	for (const auto & path : loc_paths)
	{
		const auto file = loc_file_reader::read(path, codepage);
		if (file.file_kind == loc_types::loc_file_kind_t::cel)
			continue;

		const auto source = std::string(string_utils::extract_filename(path));
		for (const auto & entry : file.entries)
			m_entries.push_back({ entry.key, entry.value, source });
	}
}

void inflection_store_t::clear()
{
	m_entries.clear();
}

std::vector<annotation_t> inflection_store_t::annotate(const std::string & translation_text) const
{
	std::vector<annotation_t> result;

	if (translation_text.empty())
		return result;

	const auto text_lower = string_utils::to_lower_utf8(translation_text);

	std::set<std::string> matched_values;
	for (const auto & entry : m_entries)
	{
		if (entry.key.empty() || entry.value.empty())
			continue;

		const auto key_lower = string_utils::to_lower_utf8(entry.key);
		const auto value_lower = string_utils::to_lower_utf8(entry.value);

		const bool key_matches = text_lower.find(key_lower) != std::string::npos;
		const bool value_matches = text_lower.find(value_lower) != std::string::npos;

		if (key_matches || value_matches)
			matched_values.insert(value_lower);
	}

	if (matched_values.empty())
		return result;

	for (const auto & entry : m_entries)
	{
		if (entry.key.empty() || entry.value.empty())
			continue;

		if (matched_values.find(string_utils::to_lower_utf8(entry.value)) == matched_values.end())
			continue;

		annotation_t annotation;
		annotation.start = 0;
		annotation.end = 0;
		annotation.kind = annotation_t::inflection_form;
		annotation.old_text = entry.key;
		annotation.new_text = entry.value;
		annotation.source = entry.source;
		result.push_back(std::move(annotation));
	}

	return result;
}
