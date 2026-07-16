#include "record_display_controller.hpp"
#include "../model/dict_document.hpp"
#include "../view/annotations_view.hpp"
#include "../view/book_preview_view.hpp"
#include "../view/editor_view.hpp"
#include "../view/history_view.hpp"
#include "../view/translation_suggestion_view.hpp"
#include "../view/validation_view.hpp"
#include <QString>
#include <QTextCursor>

record_display_controller_t::record_display_controller_t(record_display_deps_t deps)
    : m_deps(std::move(deps))
{}

void record_display_controller_t::load_record(int row, document_t * active_doc)
{
	m_deps.editor_controller.set_loading(true);

	const auto * row_data = m_deps.table_model.row_at(row);
	if (!row_data)
	{
		load_record_clear();
		return;
	}

	const auto load_result =
	    active_doc ? m_deps.editor_controller.load(*active_doc, *row_data) : editor_load_result_t {};

	if (row_data->type == rec_type_t::sctx || row_data->type == rec_type_t::bnam)
		load_record_script(row_data);
	else
		load_record_plain(row_data);

	m_deps.editor_view.translation_editor()->setReadOnly(load_result.is_read_only);

	if (row_data->type == rec_type_t::text)
		m_deps.book_preview_view.set_html(row_data->old_text, row_data->new_text);
	else if (row_data->type == rec_type_t::sctx || row_data->type == rec_type_t::bnam)
	{
		std::string full_script;
		std::string translated_script;
		auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc);
		if (dict_doc)
		{
			const auto & data = dict_doc->data();
			const auto caret_pos = row_data->key_text.find('^');
			const auto script_name = (caret_pos != std::string::npos)
			                              ? row_data->key_text.substr(0, caret_pos)
			                              : row_data->key_text;

			auto script_it = data.find(rec_type_t::script);
			if (script_it != data.end())
			{
				const auto * script_entry = script_it->second.find(script_name);
				if (script_entry)
					full_script = script_entry->old_text;
			}

			if (!full_script.empty())
			{
				translated_script = full_script;
				auto sctx_it = data.find(rec_type_t::sctx);
				if (sctx_it != data.end())
				{
					for (const auto & entry : sctx_it->second.records)
					{
						const auto entry_caret = entry.key_text.find('^');
						if (entry_caret == std::string::npos)
							continue;

						const auto entry_script = entry.key_text.substr(0, entry_caret);
						if (entry_script != script_name)
							continue;

						if (entry.new_text.empty() || entry.old_text == entry.new_text)
							continue;

						auto found = translated_script.find(entry.old_text);
						if (found != std::string::npos)
							translated_script.replace(found, entry.old_text.size(), entry.new_text);
					}
				}
			}
		}

		if (!full_script.empty())
			m_deps.book_preview_view.set_script(full_script, translated_script);
		else
			m_deps.book_preview_view.set_script(row_data->old_text, row_data->new_text);
	}
	else
		m_deps.book_preview_view.clear();

	m_deps.annotations_view.update_annotations(
	    load_result.annotations, load_result.speaker_name, load_result.gender, load_result.enchantment);

	m_deps.translation_suggestion_view.set_source_text(row_data->old_text);

	if (!load_result.details.empty())
		m_deps.editor_view.set_details(load_result.details);
	else
		m_deps.editor_view.clear_details();

	const auto & annotations = load_result.annotations;
	const auto original_lower = m_deps.editor_view.original_view()->toPlainText().toLower().toStdString();
	const auto translation_lower = m_deps.editor_view.translation_editor()->toPlainText().toLower().toStdString();

	const highlight_request_t orig_request { &annotations, true, highlight_sort_policy_t::length_first };
	auto orig_highlights = highlight_coordinator_t::find_annotation_highlights(original_lower, orig_request);

	m_deps.extra_sel_original.annotations =
	    highlight_applier_t::build_selections(m_deps.editor_view.original_view(), orig_highlights);
	m_deps.extra_sel_original.grammar.clear();
	m_deps.extra_sel_original.adapted_diff.clear();
	highlight_applier_t::apply(m_deps.editor_view.original_view(), m_deps.extra_sel_original);

	const highlight_request_t trans_request { &annotations, false, highlight_sort_policy_t::length_first };
	auto trans_highlights = highlight_coordinator_t::find_annotation_highlights(translation_lower, trans_request);

	m_deps.extra_sel_translation.annotations =
	    highlight_applier_t::build_selections(m_deps.editor_view.translation_editor(), trans_highlights);
	m_deps.extra_sel_translation.grammar =
	    m_deps.grammar_check_action.isChecked()
	        ? m_deps.grammar_checker.check(m_deps.editor_view.translation_editor(), row_data->type)
	        : QList<QTextEdit::ExtraSelection> {};
	m_deps.extra_sel_translation.adapted_diff.clear();
	highlight_applier_t::apply(m_deps.editor_view.translation_editor(), m_deps.extra_sel_translation);

	m_deps.extra_sel_adapted.annotations.clear();
	m_deps.extra_sel_adapted.grammar.clear();
	m_deps.extra_sel_adapted.adapted_diff.clear();

	if (row_data->status == status_t::adapted && !load_result.details.empty())
	{
		m_deps.extra_sel_adapted.adapted_diff =
		    m_deps.editor_view.highlight_adapted_diff(row_data->new_text, load_result.details);
	}
	else if (row_data->status == status_t::changed && !load_result.details.empty())
	{
		m_deps.extra_sel_adapted.adapted_diff =
		    m_deps.editor_view.highlight_adapted_diff(row_data->old_text, load_result.details);
	}

	highlight_applier_t::apply(m_deps.editor_view.details_view(), m_deps.extra_sel_adapted);

	const auto history = m_deps.edit_history.get_history(row_data->type, row_data->key_text);
	m_deps.history_view.update_history(history, !load_result.is_read_only);

	m_deps.editor_controller.set_loaded_text(m_deps.editor_view.translation_editor()->toPlainText());
	m_deps.editor_controller.set_current_row(row);

	update_validation();

	auto cursor = m_deps.editor_view.translation_editor()->textCursor();
	cursor.movePosition(QTextCursor::End);
	m_deps.editor_view.translation_editor()->setTextCursor(cursor);

	m_deps.editor_controller.set_loading(false);
}

void record_display_controller_t::load_record_clear()
{
	m_deps.editor_view.original_view()->clear();
	m_deps.editor_view.translation_editor()->clear();
	m_deps.validation_view.clear();
	m_deps.annotations_view.clear();
	m_deps.history_view.clear();
	m_deps.book_preview_view.clear();
	m_deps.editor_controller.set_current_row(-1);
	m_deps.editor_controller.set_loading(false);
}

void record_display_controller_t::load_record_script(const table_row_t * row_data)
{
	m_deps.editor_view.load_script_entry(row_data->old_text, row_data->new_text);
	m_deps.editor_view.translation_editor()->set_block_multiline(true);

	if (row_data->status != status_t::untranslated)
		return;

	const auto line_count = static_cast<int>(m_deps.editor_view.script_slot_count());
	QString empty_lines;
	for (int i = 1; i < line_count; ++i)
		empty_lines += '\n';

	m_deps.editor_view.translation_editor()->setPlainText(empty_lines);
}

void record_display_controller_t::load_record_plain(const table_row_t * row_data)
{
	m_deps.editor_view.original_view()->setPlainText(QString::fromStdString(row_data->old_text));
	m_deps.editor_view.clear_script_template();

	if (row_data->status == status_t::untranslated)
		m_deps.editor_view.translation_editor()->setPlainText(QString());
	else
		m_deps.editor_view.translation_editor()->setPlainText(QString::fromStdString(row_data->new_text));

	const bool block_multiline =
	    (row_data->type == rec_type_t::cell || row_data->type == rec_type_t::dial ||
	     row_data->type == rec_type_t::fnam);
	m_deps.editor_view.translation_editor()->set_block_multiline(block_multiline);
}

void record_display_controller_t::apply_translation_highlights(const table_row_t * row_data)
{
	const auto annotations = m_deps.glossary.annotate(row_data->old_text, row_data->type);
	const auto current_text = m_deps.editor_view.translation_editor()->toPlainText().toLower().toStdString();

	const highlight_request_t request { &annotations, false, highlight_sort_policy_t::hyperlink_first };
	auto highlights = highlight_coordinator_t::find_annotation_highlights(current_text, request);

	m_deps.extra_sel_translation.annotations =
	    highlight_applier_t::build_selections(m_deps.editor_view.translation_editor(), highlights);

	m_deps.extra_sel_translation.grammar =
	    m_deps.grammar_check_action.isChecked()
	        ? m_deps.grammar_checker.check(m_deps.editor_view.translation_editor(), row_data->type)
	        : QList<QTextEdit::ExtraSelection> {};

	highlight_applier_t::apply(m_deps.editor_view.translation_editor(), m_deps.extra_sel_translation);
}

void record_display_controller_t::update_validation()
{
	if (m_deps.editor_controller.current_row() < 0)
		return;

	const auto * row_data = m_deps.table_model.row_at(m_deps.editor_controller.current_row());
	if (!row_data)
		return;

	const auto current_text = m_deps.editor_view.translation_editor()->toPlainText().toStdString();
	const auto result = m_deps.byte_limit_validator.validate(row_data->type, current_text);
	m_deps.validation_view.update_validation(result);

	m_deps.extra_sel_translation.overflow.clear();

	if (result.limit > 0 && result.byte_count > result.limit)
	{
		const auto plain_text = m_deps.editor_view.translation_editor()->toPlainText();
		const auto codepage = m_deps.byte_limit_validator.codepage();
		size_t codepage_bytes = 0;
		int char_start = plain_text.length();

		for (int i = 0; i < plain_text.length(); ++i)
		{
			const auto ch_utf8 = plain_text.mid(i, 1).toStdString();
			const auto ch_encoded = encode_from_utf8(ch_utf8, codepage);
			codepage_bytes += ch_encoded.size();

			if (codepage_bytes > result.limit)
			{
				char_start = i;
				break;
			}
		}

		QTextEdit::ExtraSelection sel;
		sel.format.setBackground(QColor(180, 40, 40));
		sel.format.setForeground(QColor(255, 255, 255));
		sel.cursor = QTextCursor(m_deps.editor_view.translation_editor()->document());
		sel.cursor.setPosition(char_start);
		sel.cursor.setPosition(plain_text.length(), QTextCursor::KeepAnchor);
		m_deps.extra_sel_translation.overflow.append(sel);
	}

	highlight_applier_t::apply(m_deps.editor_view.translation_editor(), m_deps.extra_sel_translation);
}

void record_display_controller_t::update_annotations(document_t * active_doc)
{
	if (m_deps.editor_controller.current_row() < 0)
		return;

	const auto * row_data = m_deps.table_model.row_at(m_deps.editor_controller.current_row());
	if (!row_data)
		return;

	const auto annotations = m_deps.glossary.annotate(row_data->old_text, row_data->type);

	std::string speaker_name;
	std::string gender_str;
	std::string enchantment_str;

	auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc);
	if (dict_doc)
	{
		const auto & data = dict_doc->data();
		auto it = data.find(row_data->type);
		if (it != data.end() && row_data->record_index < it->second.records.size())
		{
			const auto & entry = it->second.records[row_data->record_index];
			speaker_name = entry.speaker_name;
			gender_str = entry.gender;
			enchantment_str = entry.enchantment;
		}
	}

	m_deps.annotations_view.update_annotations(annotations, speaker_name, gender_str, enchantment_str);
}
