#include "shortcuts_controller.hpp"

shortcuts_controller_t::shortcuts_controller_t(shortcuts_deps_t deps)
    : m_deps(std::move(deps))
{}

QList<int> shortcuts_controller_t::target_rows() const
{
	if (m_deps.selected_rows)
	{
		auto rows = m_deps.selected_rows();
		if (!rows.isEmpty())
			return rows;
	}

	const auto current = m_deps.editor_controller.current_row();
	if (current < 0)
		return {};

	return { current };
}

bool shortcuts_controller_t::copy_original_row(int row)
{
	const auto * row_data = m_deps.table_model.row_at(row);
	if (!row_data)
		return false;

	auto * active_doc = m_deps.active_document();
	if (!active_doc)
		return false;

	const auto result = active_doc->commit(*row_data, row_data->old_text, status_t::in_progress);
	if (!result.success)
		return false;

	m_deps.table_model.update_row(row, result.new_text, result.status);
	return true;
}

bool shortcuts_controller_t::commit_status_row(int row, status_t new_status)
{
	const auto * row_data = m_deps.table_model.row_at(row);
	if (!row_data)
		return false;

	auto * active_doc = m_deps.active_document();
	if (!active_doc)
		return false;

	const auto result = active_doc->commit_status(*row_data, new_status);
	if (!result.success)
		return false;

	m_deps.table_model.update_row(row, result.new_text, result.status);
	return true;
}

bool shortcuts_controller_t::reset_to_original_row(int row)
{
	const auto * row_data = m_deps.table_model.row_at(row);
	if (!row_data)
		return false;

	auto * active_doc = m_deps.active_document();
	if (!active_doc)
		return false;

	const auto result = active_doc->reset_to_original(*row_data);
	if (!result.success)
		return false;

	m_deps.table_model.update_row(row, result.new_text, result.status);
	return true;
}

void shortcuts_controller_t::copy_original()
{
	auto * active_doc = m_deps.active_document();
	if (!active_doc)
		return;

	bool any_changed = false;
	for (int row : target_rows())
	{
		if (copy_original_row(row))
			any_changed = true;
	}

	if (!any_changed)
		return;

	m_deps.set_unsaved_changes(active_doc->is_dirty());
	m_deps.update_status_counts();

	if (m_deps.editor_controller.current_row() >= 0)
		m_deps.load_record(m_deps.editor_controller.current_row());
}

void shortcuts_controller_t::reset_to_original(const QList<int> & rows)
{
	auto * active_doc = m_deps.active_document();
	if (!active_doc)
		return;

	bool any_changed = false;
	for (int row : rows)
	{
		if (reset_to_original_row(row))
			any_changed = true;
	}

	if (!any_changed)
		return;

	m_deps.set_unsaved_changes(active_doc->is_dirty());
	m_deps.update_status_counts();

	if (m_deps.editor_controller.current_row() >= 0)
		m_deps.load_record(m_deps.editor_controller.current_row());
}

void shortcuts_controller_t::commit_status(status_t new_status)
{
	auto * active_doc = m_deps.active_document();
	if (!active_doc)
		return;

	if (!active_doc->permissions().status_changeable)
		return;

	bool any_changed = false;
	for (int row : target_rows())
	{
		if (commit_status_row(row, new_status))
			any_changed = true;
	}

	if (!any_changed)
		return;

	m_deps.set_unsaved_changes(active_doc->is_dirty());
	m_deps.update_status_counts();

	if (m_deps.editor_controller.current_row() >= 0)
		m_deps.load_record(m_deps.editor_controller.current_row());
}
