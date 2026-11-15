#include "translation_request_controller.hpp"
#include "../editor/glossary.hpp"
#include "../editor/glossary_line_preparer.hpp"
#include "../model/document.hpp"
#include "../model/record_table_model.hpp"
#include "../model/table_row.hpp"
#include "../translator/translator.hpp"
#include "../view/editor_view.hpp"
#include "../view/translation_edit_view.hpp"
#include "../view/translation_suggestion_view.hpp"
#include "editor_controller.hpp"
#include <settings_store.hpp>
#include <string>
#include <vector>

translation_request_controller_t::translation_request_controller_t(translation_request_deps_t deps)
    : m_deps(std::move(deps))
{}

void translation_request_controller_t::translate_current_entry(document_t * active_doc)
{
	const auto * row_data = resolve_translatable_row(active_doc);
	if (!row_data)
		return;

	dispatch_translation(*row_data);
}

const table_row_t * translation_request_controller_t::resolve_translatable_row(document_t * active_doc)
{
	if (!active_doc)
	{
		m_deps.translation_suggestion_view.append_log("[error] no document loaded\n");
		return nullptr;
	}

	auto * provider = m_deps.translation_suggestion_view.active_provider();
	if (!provider || !provider->is_available())
	{
		m_deps.translation_suggestion_view.append_log("[error] translation provider not available\n");
		return nullptr;
	}

	const int current_row = m_deps.editor_controller.current_row();
	if (current_row < 0)
	{
		m_deps.translation_suggestion_view.append_log("[error] no row selected\n");
		return nullptr;
	}

	const auto * row_data = m_deps.table_model.row_at(current_row);
	if (!row_data)
		return nullptr;

	if (row_data->status != status_t::untranslated)
	{
		m_deps.translation_suggestion_view.append_log("[info] only untranslated entries can be translated\n");
		return nullptr;
	}

	return row_data;
}

void translation_request_controller_t::dispatch_translation(const table_row_t & row_data)
{
	m_deps.translation_suggestion_view.set_source_text(row_data.old_text);
	m_deps.translation_suggestion_view.set_target_language(m_deps.settings.native_language());

	if (!m_deps.editor_view.has_script_template())
	{
		m_deps.translation_suggestion_view.request_translation_segments(row_data.old_text);
		return;
	}

	const auto lines = m_deps.editor_view.original_view()->toPlainText().split('\n');
	std::vector<std::string> source_lines;

	for (const auto & line : lines)
		source_lines.push_back(line.toStdString());

	const auto prepared_lines = glossary_line_preparer::prepare_lines(
	    source_lines, [this](const std::string & source) { return m_deps.glossary.apply_glossary(source); });

	m_deps.translation_suggestion_view.request_translation_lines(prepared_lines);
}
