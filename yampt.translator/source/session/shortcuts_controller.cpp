#include "shortcuts_controller.hpp"

shortcuts_controller_t::shortcuts_controller_t(shortcuts_deps_t deps)
    : m_deps(std::move(deps))
{}

void shortcuts_controller_t::copy_original()
{
	if (m_deps.editor_controller.current_row() < 0)
		return;

	auto * active_doc = m_deps.active_document();
	if (!active_doc)
		return;

	const auto * row_data = m_deps.table_model.row_at(m_deps.editor_controller.current_row());
	if (!row_data)
		return;

	const auto result = active_doc->reset_to_original(*row_data);
	if (!result.success)
		return;

	m_deps.table_model.update_row(m_deps.editor_controller.current_row(), result.new_text, result.status);
	m_deps.set_unsaved_changes(true);
	m_deps.update_status_counts();
	m_deps.load_record(m_deps.editor_controller.current_row());
}

void shortcuts_controller_t::commit_status(status_t new_status)
{
	if (m_deps.editor_controller.current_row() < 0)
		return;

	auto * active_doc = m_deps.active_document();
	if (!active_doc)
		return;

	if (!active_doc->permissions().status_changeable)
		return;

	const auto * row_data = m_deps.table_model.row_at(m_deps.editor_controller.current_row());
	if (!row_data)
		return;

	const auto result = active_doc->commit_status(*row_data, new_status);
	if (!result.success)
		return;

	m_deps.table_model.update_row(m_deps.editor_controller.current_row(), result.new_text, result.status);
	m_deps.set_unsaved_changes(true);
	m_deps.update_status_counts();
}
