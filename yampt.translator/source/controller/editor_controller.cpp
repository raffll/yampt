#include "editor_controller.hpp"
#include "../model/dict_document.hpp"
#include "../model/document.hpp"
#include "../model/record_table_model.hpp"

editor_controller_t::editor_controller_t(glossary_t & annotations)
    : m_annotations(annotations)
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
	result.annotations = m_annotations.annotate(row.old_text);

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
