#include "editor_controller.hpp"
#include "../model/dict_document.hpp"
#include "../model/document.hpp"
#include "../model/record_table_model.hpp"
#include "../model/yaml_document.hpp"
#include <optional>

editor_controller_t::editor_controller_t(
    edit_history_t & history,
    byte_limit_validator_t & validation,
    glossary_t & annotations)
    : m_history(history)
    , m_validation(validation)
    , m_annotations(annotations)
{}

int editor_controller_t::current_row() const
{
	return m_current_row;
}

const QString & editor_controller_t::loaded_text() const
{
	return m_loaded_text;
}

bool editor_controller_t::is_loading() const
{
	return m_loading_record;
}

void editor_controller_t::set_current_row(int row)
{
	m_current_row = row;
}

void editor_controller_t::set_loaded_text(const QString & text)
{
	m_loaded_text = text;
}

void editor_controller_t::set_loading(bool loading)
{
	m_loading_record = loading;
}

void editor_controller_t::set_pending_status(status_t status)
{
	m_pending_status = status;
}

std::optional<status_t> editor_controller_t::take_pending_status()
{
	auto result = m_pending_status;
	m_pending_status = std::nullopt;
	return result;
}

editor_load_result_t editor_controller_t::load(document_t & doc, const table_row_t & row)
{
	editor_load_result_t result;
	result.old_text = row.old_text;
	result.new_text = row.new_text;
	result.status = row.status;
	result.is_read_only = doc.is_read_only();
	result.annotations = m_annotations.annotate(row.old_text, row.type);

	auto * dict_doc = dynamic_cast<dict_document_t *>(&doc);
	if (!dict_doc)
		return result;

	const auto & data = dict_doc->data();
	auto it = data.find(row.type);
	if (it == data.end())
		return result;

	if (row.record_index >= it->second.records.size())
		return result;

	const auto & entry = it->second.records[row.record_index];
	result.speaker_name = entry.speaker_name;
	result.gender = entry.gender;
	result.enchantment = entry.enchantment;
	result.details = entry.details;

	return result;
}

commit_result_t editor_controller_t::commit(
    dict_document_t & doc,
    const table_row_t & row,
    const std::string & new_text)
{
	commit_result_t result;
	result.new_text = new_text;

	auto & data = doc.data_mut();
	auto it = data.find(row.type);
	if (it == data.end() || row.record_index >= it->second.records.size())
		return result;

	auto & entry = it->second.records[row.record_index];

	const auto old_status = entry.status;
	m_history.record_change(row.type, row.key_text, m_loaded_text.toStdString(), new_text, old_status);

	const auto validation = m_validation.validate(row.type, new_text);
	const auto pending = take_pending_status();
	const auto commit_status =
	    (validation.level == validation_level_t::error) ? status_t::error
	                                                    : pending.value_or(status_t::in_progress);

	entry.new_text = new_text;
	entry.status = commit_status;
	doc.set_dirty(true);
	doc.modified_records_insert(row.type, row.record_index);

	m_annotations.update_term(row.type, entry.old_text, new_text);

	int propagated = 0;
	if (commit_status != status_t::model && new_text != entry.old_text)
	{
		propagated = propagate(doc, entry.old_text, new_text);
		if (propagated > 0)
			entry.status = status_t::propagated;
	}

	result.status = entry.status;
	result.propagated_count = propagated;
	result.success = true;
	return result;
}

commit_result_t editor_controller_t::commit_status(dict_document_t & doc, const table_row_t & row, status_t new_status)
{
	commit_result_t result;

	auto & data = doc.data_mut();
	auto it = data.find(row.type);
	if (it == data.end() || row.record_index >= it->second.records.size())
		return result;

	auto & entry = it->second.records[row.record_index];
	entry.status = new_status;
	doc.set_dirty(true);
	doc.modified_records_insert(row.type, row.record_index);

	result.new_text = entry.new_text;
	result.status = new_status;
	result.success = true;
	return result;
}

void editor_controller_t::copy_original(dict_document_t & doc, const table_row_t & row)
{
	auto & data = doc.data_mut();
	auto it = data.find(row.type);
	if (it == data.end() || row.record_index >= it->second.records.size())
		return;

	auto & entry = it->second.records[row.record_index];
	entry.new_text = entry.old_text;
	entry.status = status_t::in_progress;
	doc.set_dirty(true);
	doc.modified_records_insert(row.type, row.record_index);
}

void editor_controller_t::clear_and_untranslate(dict_document_t & doc, const table_row_t & row)
{
	auto & data = doc.data_mut();
	auto it = data.find(row.type);
	if (it == data.end() || row.record_index >= it->second.records.size())
		return;

	auto & entry = it->second.records[row.record_index];
	entry.new_text = entry.old_text;
	entry.status = status_t::untranslated;
	doc.set_dirty(true);
	doc.modified_records_insert(row.type, row.record_index);
}

int editor_controller_t::propagate(dict_document_t & doc, const std::string & old_text, const std::string & new_text)
{
	auto trimmed_source = old_text;
	while (!trimmed_source.empty() && (trimmed_source.front() == ' ' || trimmed_source.front() == '\t'))
		trimmed_source.erase(trimmed_source.begin());

	while (!trimmed_source.empty() && (trimmed_source.back() == ' ' || trimmed_source.back() == '\t'))
		trimmed_source.pop_back();

	int count = 0;
	for (auto & [type, chapter] : doc.data_mut())
	{
		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			auto & entry = chapter.records[i];

			if (entry.new_text == new_text)
				continue;

			auto trimmed = entry.old_text;
			while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
				trimmed.erase(trimmed.begin());

			while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t'))
				trimmed.pop_back();

			if (trimmed != trimmed_source)
				continue;

			entry.new_text = new_text;
			entry.status = status_t::propagated;
			doc.modified_records_insert(type, i);
			++count;
		}
	}

	if (count > 0)
		doc.set_dirty(true);

	return count;
}

dict_commit_result_t editor_controller_t::commit_dict_full(
    dict_document_t & doc,
    const table_row_t & row,
    const std::string & new_text)
{
	dict_commit_result_t result;
	result.base_result = commit(doc, row, new_text);
	if (!result.base_result.success)
		return result;

	if (result.base_result.propagated_count <= 0)
		return result;

	const auto & data = doc.data();
	for (auto & [type, chapter] : data)
	{
		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			if (!doc.modified_records().count({ type, i }))
				continue;

			if (type == row.type && i == row.record_index)
				continue;

			const auto & record = chapter.records[i];
			if (record.status != status_t::propagated)
				continue;

			if (record.new_text != new_text)
				continue;

			result.propagated_rows.push_back(static_cast<int>(i));
		}
	}

	return result;
}

void editor_controller_t::commit_yaml(document_t & doc, const table_row_t & row, const std::string & new_text)
{
	doc.commit_edit(row.type, row.record_index, new_text);

	auto * yaml_doc = dynamic_cast<yaml_document_t *>(&doc);
	if (yaml_doc)
		yaml_doc->save_tmp();
}

void editor_controller_t::sync_propagated_rows(record_table_model_t & model, dict_document_t & doc)
{
	const auto & data = doc.data();

	for (int i = 0; i < model.rowCount(); ++i)
	{
		if (i == m_current_row)
			continue;

		const auto * row = model.row_at(i);
		if (!row)
			continue;

		auto chap_it = data.find(row->type);
		if (chap_it == data.end())
			continue;

		if (row->record_index >= chap_it->second.records.size())
			continue;

		const auto & record = chap_it->second.records[row->record_index];
		if (record.new_text != row->new_text || record.status != row->status)
			model.update_row(i, record.new_text, record.status);
	}
}
