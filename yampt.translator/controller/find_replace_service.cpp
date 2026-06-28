#include "find_replace_service.hpp"

#include <QString>

find_replace_service_t::find_replace_service_t(row_provider_t & provider, document_t *& active_doc)
    : provider_(provider)
    , active_doc_(active_doc)
{}

std::optional<find_replace_service_t::search_params_t> find_replace_service_t::build_search_params(
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

	params.lower_query = case_sensitive
	    ? QString::fromStdString(query)
	    : QString::fromStdString(query).toLower();

	return params;
}

bool find_replace_service_t::matches_query(
    const std::string & text_value,
    const search_params_t & params)
{
	if (params.regex_opt)
		return std::regex_search(text_value, *params.regex_opt);

	auto haystack = QString::fromStdString(text_value);
	if (!params.case_sensitive)
		haystack = haystack.toLower();

	return haystack.contains(params.lower_query);
}

std::optional<std::string> find_replace_service_t::apply_replacement(
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
		const auto & found_index = q_text.indexOf(
		    q_query, position,
		    params.case_sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

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

find_replace_service_t::find_result_t find_replace_service_t::find_next(
    const std::string & query,
    bool case_sensitive,
    bool regex_mode,
    int current_row)
{
	if (query.empty())
		return {};

	const int count = provider_.row_count();
	if (count == 0)
		return {};

	const auto & params = build_search_params(query, "", case_sensitive, regex_mode);
	if (!params)
		return {};

	for (int i = 1; i <= count; ++i)
	{
		const int row_index = (current_row + i) % count;
		const auto * row_data = provider_.row_at(row_index);
		if (!row_data)
			continue;

		if (matches_query(row_data->new_text, *params))
			return { row_index, true };
	}

	return {};
}

find_replace_service_t::replace_result_t find_replace_service_t::replace_current(
    const std::string & query,
    const std::string & replacement,
    bool case_sensitive,
    bool regex_mode,
    int current_row)
{
	if (query.empty())
		return {};

	if (current_row < 0)
		return {};

	auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
	if (!dict_doc)
		return {};

	const auto * row_data = provider_.row_at(current_row);
	if (!row_data)
		return {};

	auto & chapter_data = dict_doc->data_mut();
	auto it_chapter = chapter_data.find(row_data->type);
	if (it_chapter == chapter_data.end())
		return {};

	if (row_data->record_index >= it_chapter->second.records.size())
		return {};

	const auto & params = build_search_params(query, replacement, case_sensitive, regex_mode);
	if (!params)
		return {};

	auto & entry = it_chapter->second.records[row_data->record_index];
	const auto & result = apply_replacement(entry.new_text, *params);
	if (!result)
		return {};

	entry.new_text = *result;
	entry.status = "in_progress";
	dict_doc->set_dirty(true);
	dict_doc->modified_records_insert(row_data->type, row_data->record_index);

	return { true, entry.new_text, entry.status };
}

find_replace_service_t::replace_all_result_t find_replace_service_t::replace_all(
    const std::string & query,
    const std::string & replacement,
    bool case_sensitive,
    bool regex_mode)
{
	if (query.empty())
		return {};

	auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
	if (!dict_doc)
		return {};

	const auto & params = build_search_params(query, replacement, case_sensitive, regex_mode);
	if (!params)
		return {};

	int replaced_count = 0;

	for (auto & [type, chapter] : dict_doc->data_mut())
	{
		for (size_t index = 0; index < chapter.records.size(); ++index)
		{
			auto & entry = chapter.records[index];
			const auto & result = apply_replacement(entry.new_text, *params);
			if (!result)
				continue;

			entry.new_text = *result;
			entry.status = "in_progress";
			dict_doc->modified_records_insert(type, index);
			++replaced_count;
		}
	}

	if (replaced_count > 0)
		dict_doc->set_dirty(true);

	return { replaced_count };
}
