#include "editor_controller.hpp"
#include "dict_document.hpp"
#include "document.hpp"

editor_controller_t::editor_controller_t(
    history_manager_t & history,
    validation_manager_t & validation,
    annotation_manager_t & annotations)
    : history_(history)
    , validation_(validation)
    , annotations_(annotations)
{}

int editor_controller_t::current_row() const
{
	return current_row_;
}

const QString & editor_controller_t::loaded_text() const
{
	return loaded_text_;
}

bool editor_controller_t::is_loading() const
{
	return loading_record_;
}

void editor_controller_t::set_current_row(int row)
{
	current_row_ = row;
}

void editor_controller_t::set_loaded_text(const QString & text)
{
	loaded_text_ = text;
}

void editor_controller_t::set_loading(bool loading)
{
	loading_record_ = loading;
}

editor_load_result_t editor_controller_t::load(document_t & doc, const table_row_t & row)
{
	editor_load_result_t result;
	result.old_text = row.old_text;
	result.new_text = row.new_text;
	result.status = row.status;
	result.is_read_only = doc.is_read_only();
	result.annotations = annotations_.annotate(row.old_text, row.type);

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

	history_.record_change(row.type, row.key_text, loaded_text_.toStdString(), new_text);

	const auto validation = validation_.validate(row.type, new_text);
	const std::string commit_status = (validation.level == validation_level_t::error) ? "error" : "in_progress";

	entry.new_text = new_text;
	entry.status = commit_status;
	doc.set_dirty(true);
	doc.modified_records_insert(row.type, row.record_index);

	annotations_.update_term(row.type, entry.old_text, new_text);

	int propagated = 0;
	if (new_text != entry.old_text)
	{
		propagated = propagate(doc, entry.old_text, new_text);
		if (propagated > 0)
			entry.status = tools_t::status_t::propagated;
	}

	result.status = entry.status;
	result.propagated_count = propagated;
	result.success = true;
	return result;
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
			entry.status = tools_t::status_t::propagated;
			doc.modified_records_insert(type, i);
			++count;
		}
	}

	if (count > 0)
		doc.set_dirty(true);

	return count;
}
