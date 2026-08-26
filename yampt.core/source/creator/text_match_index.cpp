#include "text_match_index.hpp"

void text_match_index_t::build(const dict_t & base_dict)
{
	m_index.clear();
	m_conflicts.clear();
	m_first_translation.clear();

	for (const auto & [type, chapter] : base_dict)
	{
		for (const auto & entry : chapter.records)
		{
			if (entry.old_text.empty())
				continue;

			if (entry.new_text == entry.old_text)
				continue;

			if (entry.status != status_t::translated)
				continue;

			auto it = m_index.find(entry.old_text);
			if (it == m_index.end())
			{
				m_index[entry.old_text] = &entry;
				continue;
			}

			if (it->second == nullptr)
			{
				auto it_conflict = m_conflicts.find(entry.old_text);
				if (it_conflict != m_conflicts.end())
				{
					bool already_listed = false;
					size_t search_pos = 0;
					while (search_pos < it_conflict->second.size())
					{
						const auto pipe_pos = it_conflict->second.find('|', search_pos);
						const auto token = it_conflict->second.substr(
						    search_pos, pipe_pos == std::string::npos ? std::string::npos : pipe_pos - search_pos);

						if (token == entry.new_text)
						{
							already_listed = true;
							break;
						}

						search_pos = (pipe_pos == std::string::npos) ? it_conflict->second.size() : pipe_pos + 1;
					}

					if (!already_listed)
						it_conflict->second += "|" + entry.new_text;
				}
				continue;
			}

			if (it->second->new_text != entry.new_text)
			{
				m_first_translation[entry.old_text] = it->second->new_text;
				m_conflicts[entry.old_text] = it->second->new_text + "|" + entry.new_text;
				it->second = nullptr;
			}
		}
	}
}

text_match_index_t::find_outcome_t text_match_index_t::find(const std::string & old_text) const
{
	auto it = m_index.find(old_text);
	if (it == m_index.end())
		return { find_result_t::not_found, {}, {} };

	if (it->second != nullptr)
		return { find_result_t::found, it->second->new_text, {} };

	auto it_conflict = m_conflicts.find(old_text);
	if (it_conflict == m_conflicts.end())
		return { find_result_t::not_found, {}, {} };

	auto it_first = m_first_translation.find(old_text);
	const auto & translation = (it_first != m_first_translation.end()) ? it_first->second : old_text;

	return { find_result_t::ambiguous, translation, it_conflict->second };
}
