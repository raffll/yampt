#include "find_replace.hpp"
#include "edit_history.hpp"
#include <QString>

find_replace_t::find_replace_t(row_source_t & source, document_t *& active_doc, edit_history_t & history)
    : m_source(source)
    , m_active_doc(active_doc)
    , m_history(history)
{}

std::optional<find_replace_t::search_params_t> find_replace_t::build_search_params(
    const std::string & query,
    const std::string & replacement,
    bool case_sensitive,
    bool regex_mode)
{
	search_params_t params;
	params.query = query;
	params.replacement = replacement;
	params.case_sensitive = case_sensitive;

	if (regex_mode)
	{
		auto flags = std::regex_constants::ECMAScript;
		if (!case_sensitive)
			flags |= std::regex_constants::icase;

		try
		{
			params.regex_opt.emplace(query, flags);
		}
		catch (...)
		{
			return std::nullopt;
		}
	}

	params.lower_query = case_sensitive ? QString::fromStdString(query) : QString::fromStdString(query).toLower();

	return params;
}

std::optional<std::string> find_replace_t::apply_replacement(
    const std::string & source_text,
    const search_params_t & params)
{
	if (params.regex_opt)
	{
		const auto & result = std::regex_replace(source_text, *params.regex_opt, params.replacement);
		if (result == source_text)
			return std::nullopt;

		return result;
	}

	auto q_text = QString::fromStdString(source_text);
	const auto & q_query = QString::fromStdString(params.query);
	const auto & q_replacement = QString::fromStdString(params.replacement);
	int position = 0;
	bool changed = false;

	while (true)
	{
		const auto & found_index =
		    q_text.indexOf(q_query, position, params.case_sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

		if (found_index < 0)
			break;

		q_text.replace(found_index, q_query.length(), q_replacement);
		position = found_index + q_replacement.length();
		changed = true;
	}

	if (!changed)
		return std::nullopt;

	return q_text.toStdString();
}

find_replace_t::replace_all_result_t find_replace_t::replace_all(
    const std::string & query,
    const std::string & replacement,
    bool case_sensitive,
    bool regex_mode)
{
	if (query.empty())
		return {};

	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (!dict_doc)
		return {};

	const auto & params = build_search_params(query, replacement, case_sensitive, regex_mode);
	if (!params)
		return {};

	int replaced_count = 0;
	const int visible_count = m_source.row_count();

	for (int row = 0; row < visible_count; ++row)
	{
		const auto * row_data = m_source.row_at(row);
		if (!row_data)
			continue;

		auto & chapter_data = dict_doc->data_mut();
		auto it_chapter = chapter_data.find(row_data->type);
		if (it_chapter == chapter_data.end())
			continue;

		if (row_data->record_index >= it_chapter->second.records.size())
			continue;

		auto & entry = it_chapter->second.records[row_data->record_index];
		const auto & result = apply_replacement(entry.new_text, *params);
		if (!result)
			continue;

		m_history.record_change(row_data->type, row_data->key_text, entry.new_text, *result, entry.status);

		entry.new_text = *result;
		entry.status = status_t::replaced;
		dict_doc->modified_records_insert(row_data->type, row_data->record_index);
		++replaced_count;
	}

	if (replaced_count > 0)
		dict_doc->set_dirty(true);

	return { replaced_count };
}

bool find_replace_t::replace_one(
    int row,
    const std::string & query,
    const std::string & replacement,
    bool case_sensitive,
    bool regex_mode)
{
	if (query.empty())
		return false;

	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (!dict_doc)
		return false;

	const auto & params = build_search_params(query, replacement, case_sensitive, regex_mode);
	if (!params)
		return false;

	const auto * row_data = m_source.row_at(row);
	if (!row_data)
		return false;

	auto & chapter_data = dict_doc->data_mut();
	auto it_chapter = chapter_data.find(row_data->type);
	if (it_chapter == chapter_data.end())
		return false;

	if (row_data->record_index >= it_chapter->second.records.size())
		return false;

	auto & entry = it_chapter->second.records[row_data->record_index];
	const auto & result = apply_replacement(entry.new_text, *params);
	if (!result)
		return false;

	m_history.record_change(row_data->type, row_data->key_text, entry.new_text, *result, entry.status);

	entry.new_text = *result;
	entry.status = status_t::replaced;
	dict_doc->modified_records_insert(row_data->type, row_data->record_index);
	dict_doc->set_dirty(true);

	return true;
}
