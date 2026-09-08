#include "merge_controller.hpp"
#include "../patcher/patch_builder.hpp"
#include "../session/merged_patch_name.hpp"
#include "../session/plugin_session.hpp"
#include "../view/nav_tree_view.hpp"
#include "../view/record_view.hpp"
#include <io/binary_file_io.hpp>
#include <scanner/auto_merge.hpp>
#include <scanner/merge_patch_ops.hpp>
#include <scanner/sub_record_merge.hpp>
#include <utility/app_logger.hpp>
#include <utility/record_behavior.hpp>
#include <filesystem>
#include <set>
#include <settings_store.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>

namespace {

std::set<std::string> parse_sub_record_rules(const std::string & input)
{
	std::set<std::string> result;
	size_t start = 0;

	while (start < input.size())
	{
		const auto comma = input.find(',', start);
		const auto end = (comma == std::string::npos) ? input.size() : comma;

		auto token_start = start;
		while (token_start < end && input[token_start] == ' ')
			++token_start;

		auto token_end = end;
		while (token_end > token_start && input[token_end - 1] == ' ')
			--token_end;

		if (token_end > token_start)
			result.insert(input.substr(token_start, token_end - token_start));

		start = (comma == std::string::npos) ? input.size() : comma + 1;
	}

	return result;
}

} // namespace

merge_controller_t::merge_controller_t(
    plugin_session_t & session,
    record_view_t & record_view,
    nav_tree_view_t & nav_view,
    settings_store_t & settings,
    log_fn_t log_fn)
    : m_session(session)
    , m_record_view(record_view)
    , m_nav_view(nav_view)
    , m_settings(settings)
    , m_log(std::move(log_fn))
{}

void merge_controller_t::set_refresh_callback(refresh_fn_t refresh_fn)
{
	m_refresh = std::move(refresh_fn);
}

void merge_controller_t::set_lock_changed_callback(lock_changed_fn_t lock_changed_fn)
{
	m_lock_changed = std::move(lock_changed_fn);
}

void merge_controller_t::set_record_removal_callback(record_removal_fn_t removal_fn)
{
	m_record_removal = std::move(removal_fn);
}

void merge_controller_t::set_progress_callback(progress_fn_t progress_fn)
{
	m_progress = std::move(progress_fn);
}

void merge_controller_t::set_phase_callback(phase_fn_t phase_fn)
{
	m_phase = std::move(phase_fn);
}

bool merge_controller_t::confirm_merged_patch_regeneration(int merged_idx)
{
	const bool merged_exists = merged_idx >= 0 || (m_session.scan().has_active() &&
	                                               m_session.scan().plugin_filename(m_session.scan().active_plugin_index()) ==
	                                                   merged_patch::filename);

	if (!merged_exists)
		return true;

	const auto answer = QMessageBox::question(
	    nullptr,
	    QCoreApplication::translate("yEditor", "Regenerate Merged Patch"),
	    QCoreApplication::translate(
	        "yEditor", "This will regenerate the merged patch and discard manual changes. Continue?"),
	    QMessageBox::Yes | QMessageBox::No,
	    QMessageBox::No);

	return answer == QMessageBox::Yes;
}

bool merge_controller_t::prompt_save_before_merge()
{
	if (!m_session.has_any_unsaved())
		return true;

	const auto answer = QMessageBox::question(
	    nullptr,
	    QCoreApplication::translate("yEditor", "Unsaved Changes"),
	    QCoreApplication::translate("yEditor", "Save unsaved plugins before creating the merged patch?"),
	    QMessageBox::Save | QMessageBox::Cancel,
	    QMessageBox::Save);

	if (answer == QMessageBox::Cancel)
		return false;

	save_all_dirty();
	return true;
}

void merge_controller_t::activate_merged_patch_target(int merged_idx)
{
	m_session.register_created_plugin(std::string(merged_patch::filename));

	if (merged_idx >= 0)
		m_session.scan().set_active_from_loaded(merged_idx);
	else
		m_session.scan().set_active_plugin(std::string(merged_patch::filename));

	load_merged_patch_locks();
}

void merge_controller_t::rebuild_merged_patch_conflicts()
{
	if (m_phase)
		m_phase(QCoreApplication::translate("yEditor", "Computing conflicts...").toStdString());

	if (m_progress)
		m_progress(0, 1);

	m_session.scan().rebuild_conflicts(
	    [this](size_t done, size_t total)
	{
		if (m_progress)
			m_progress(static_cast<int>(done), static_cast<int>(total));
	});

	m_nav_view.rebuild_preserving_state();
}

bool merge_controller_t::create_merged_patch()
{
	if (!prompt_save_before_merge())
		return false;

	if (m_session.scan().plugin_count() < 1)
	{
		m_log("[error] no plugins loaded");
		return false;
	}

	const int merged_idx = find_merged_patch_index();
	if (!confirm_merged_patch_regeneration(merged_idx))
		return false;

	activate_merged_patch_target(merged_idx);

	if (m_phase)
		m_phase(QCoreApplication::translate("yEditor", "Merging records...").toStdString());

	create_merge_records();
	rebuild_merged_patch_conflicts();

	m_log("[info] merged patch record count: " + std::to_string(m_session.scan().active_record_count()));
	save_active_plugin();
	return true;
}

void merge_controller_t::load_existing_merged_patch()
{
	const auto path = resolve_active_output_path();
	if (path.empty())
		return;

	if (!std::filesystem::exists(path))
	{
		m_log("[warning] merged patch not found: " + path);
		return;
	}

	if (m_session.scan().has_active())
		return;

	auto merge_filename = std::filesystem::path(path).filename().string();

	for (int i = 0; i < static_cast<int>(m_session.scan().plugin_count()); ++i)
	{
		if (m_session.scan().plugin_filename(i) == merge_filename)
		{
			m_session.scan().set_active_from_loaded(i);
			load_merged_patch_locks();
			m_session.scan().rebuild_conflicts();
			m_log("[info] tagged existing plugin as merge: " + merge_filename);
			return;
		}
	}

	try
	{
		m_session.scan().load_plugin(path);
		const int loaded_idx = static_cast<int>(m_session.scan().plugin_count()) - 1;
		m_session.scan().set_active_from_loaded(loaded_idx);
		load_merged_patch_locks();
		m_session.scan().rebuild_conflicts();
		m_log("[info] loaded existing merged patch: " + path);
	}
	catch (const std::exception & error)
	{
		m_log("[error] cannot load merged patch: " + std::string(error.what()));
	}
}

bool merge_controller_t::prompt_save_active_before_switch()
{
	if (!m_session.scan().has_active())
		return true;

	const int active_idx = m_session.scan().active_plugin_index();
	if (active_idx < 0 || !m_session.is_plugin_dirty(active_idx))
		return true;

	const auto answer = QMessageBox::question(
	    nullptr,
	    QCoreApplication::translate("yEditor", "Save Active Plugin"),
	    QCoreApplication::translate(
	        "yEditor", "Save changes to the current active plugin before switching?"),
	    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
	    QMessageBox::Save);

	if (answer == QMessageBox::Cancel)
		return false;

	if (answer == QMessageBox::Save)
		save_active_plugin();

	return true;
}

void merge_controller_t::create_new_plugin(const std::string & filename)
{
	if (filename.empty())
		return;

	if (!prompt_save_active_before_switch())
		return;

	m_session.register_created_plugin(filename);
	m_session.scan().set_active_plugin(filename);
	m_session.scan().set_active_locks({});

	save_active_plugin();

	const auto saved_path = resolve_active_output_path();
	if (!saved_path.empty())
		m_session.scan().reload_active_plugin(saved_path);

	m_session.scan().rebuild_conflicts();

	if (m_refresh)
		m_refresh();
	else
		m_nav_view.rebuild_preserving_state();

	m_log("[info] created new plugin: " + filename);
}

void merge_controller_t::set_active_plugin(int plugin_idx)
{
	if (plugin_idx < 0 || plugin_idx >= static_cast<int>(m_session.scan().plugin_count()))
		return;

	if (m_session.scan().is_active_plugin(plugin_idx))
		return;

	if (!prompt_save_active_before_switch())
		return;

	m_session.scan().set_active_from_loaded(plugin_idx);

	if (m_session.scan().plugin_filename(plugin_idx) == merged_patch::filename)
		load_merged_patch_locks();
	else
		m_session.scan().set_active_locks({});

	m_session.scan().rebuild_conflicts();

	if (m_refresh)
		m_refresh();
	else
		m_nav_view.rebuild_preserving_state();

	m_log("[info] set active plugin: " + m_session.scan().plugin_filename(plugin_idx));
}

void merge_controller_t::copy_whole_record(int plugin_idx, const std::string & rec_type, const std::string & record_id)
{
	const auto * entry = m_session.scan().find(rec_type, record_id);
	if (!entry)
	{
		m_log("[warning] copy_whole_record: no conflict entry for " + rec_type + ":" + record_id);
		return;
	}

	bool copied = false;
	for (const auto & version : entry->versions)
	{
		if (version.plugin_idx != plugin_idx)
			continue;

		m_session.scan().copy_record_to_active(plugin_idx, version.record_index);
		copied = true;
		break;
	}

	if (!copied)
	{
		m_log(
		    "[warning] copy_whole_record: plugin " + std::to_string(plugin_idx) + " has no version of " + rec_type +
		    ":" + record_id);
		return;
	}

	m_log("[info] copied record to active plugin (" + rec_type + ":" + record_id + ")");
	refresh_after_active_edit(rec_type, record_id);
	save_active_plugin();
}

void merge_controller_t::copy_cell_record(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    const QModelIndex & clicked_index,
    int clicked_col)
{
	const auto source_content = read_source_content(plugin_idx, rec_type, record_id);
	if (source_content.empty())
	{
		m_log(
		    "[warning] copy_cell_record: empty source content for " + rec_type + ":" + record_id + " in plugin " +
		    std::to_string(plugin_idx));
		return;
	}

	auto partition = sub_record_merge_t::partition_cell(source_content);

	uint32_t selected_frmr = 0;
	bool found_frmr = false;

	QModelIndex walk = clicked_index;
	while (walk.isValid())
	{
		const auto * walk_node = m_record_view.model()->node_from_index(walk);
		if (walk_node && walk_node->type == "FRMR" && walk_node->size == 0)
		{
			if (clicked_col >= 0 && clicked_col < static_cast<int>(walk_node->values.size()) &&
			    !walk_node->values[clicked_col].empty())
			{
				try
				{
					selected_frmr = static_cast<uint32_t>(std::stoul(walk_node->values[clicked_col]));
					found_frmr = true;
				}
				catch (const std::exception & error)
				{
					m_log("[error] invalid FRMR index: " + std::string(error.what()));
				}
			}

			break;
		}

		walk = walk.parent();
	}

	sub_record_sequence_t output = partition.header;

	if (found_frmr)
	{
		for (const auto & group : partition.groups)
		{
			if (group.frmr_index != selected_frmr)
				continue;

			output.insert(output.end(), group.sub_records.begin(), group.sub_records.end());
			break;
		}
	}

	const auto result = sub_record_merge_t::reconstruct_record(source_content, output);
	m_session.scan().copy_record_to_active_raw(rec_type, record_id, result);
	m_log("[info] copied CELL record to active plugin (" + record_id + ")");

	refresh_after_active_edit(rec_type, record_id);
	save_active_plugin();
}

void merge_controller_t::copy_sub_record(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & sub_type,
    int binary_idx)
{
	const auto source_content = read_source_content(plugin_idx, rec_type, record_id);
	if (source_content.empty())
	{
		m_log(
		    "[warning] copy_sub_record: empty source content for " + rec_type + ":" + record_id + " in plugin " +
		    std::to_string(plugin_idx));
		return;
	}

	const auto merge_content = ensure_active_record(plugin_idx, rec_type, record_id, source_content);
	if (merge_content.empty())
	{
		m_log("[warning] copy_sub_record: could not ensure active record for " + rec_type + ":" + record_id);
		return;
	}

	const auto result = merge_patch_ops_t::patch_sub_record(merge_content, source_content, sub_type, binary_idx);
	if (!result.success)
	{
		m_log(
		    "[warning] copy_sub_record: patch failed for " + sub_type + " (binary_idx=" +
		    std::to_string(binary_idx) + ") in " + rec_type + ":" + record_id);
		return;
	}

	m_session.scan().copy_record_to_active_raw(rec_type, record_id, result.content);
	m_log(
	    "[info] copied " + sub_type + " from " + m_session.scan().plugin_filename(plugin_idx) + " to active plugin (" +
	    rec_type + ":" + record_id + ")");

	refresh_after_active_edit(rec_type, record_id);
	save_active_plugin();
}

void merge_controller_t::copy_group(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    int group_row_idx)
{
	const auto source_content = read_source_content(plugin_idx, rec_type, record_id);
	if (source_content.empty())
	{
		m_log(
		    "[warning] copy_group: empty source content for " + rec_type + ":" + record_id + " in plugin " +
		    std::to_string(plugin_idx));
		return;
	}

	const auto merge_content = ensure_active_record(plugin_idx, rec_type, record_id, source_content);
	if (merge_content.empty())
	{
		m_log("[warning] copy_group: could not ensure active record for " + rec_type + ":" + record_id);
		return;
	}

	const int column = find_plugin_column(plugin_idx);
	const auto & visible = m_record_view.model()->rows();
	if (group_row_idx < 0 || group_row_idx >= static_cast<int>(visible.size()))
	{
		m_log(
		    "[warning] copy_group: group_row_idx " + std::to_string(group_row_idx) + " out of range (visible=" +
		    std::to_string(visible.size()) + ")");
		return;
	}

	const auto & group_row = visible[group_row_idx];

	if (column < 0 || column >= static_cast<int>(group_row.binary_ranges.size()))
	{
		m_log("[warning] copy_group: column " + std::to_string(column) + " out of range for group binary ranges");
		return;
	}

	const auto & source_range = group_row.binary_ranges[column];
	if (source_range.start < 0)
	{
		m_log("[error] copy_group: no binary range for column " + std::to_string(column));
		return;
	}

	const auto source_subs = sub_record_merge_t::parse_sub_records(source_content);
	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);

	if (source_range.end_pos > static_cast<int>(source_subs.size()))
	{
		m_log("[error] copy_group: range exceeds source sub-records");
		return;
	}

	sub_record_sequence_t source_group(
	    source_subs.begin() + source_range.start, source_subs.begin() + source_range.end_pos);

	merge_subs.insert(merge_subs.end(), source_group.begin(), source_group.end());

	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	m_session.scan().copy_record_to_active_raw(rec_type, record_id, patched);
	m_log(
	    "[info] copied group \"" + group_row.label + "\" from " + m_session.scan().plugin_filename(plugin_idx) +
	    " to active plugin (" + rec_type + ":" + record_id + ")");

	refresh_after_active_edit(rec_type, record_id);
	save_active_plugin();
}

void merge_controller_t::copy_field(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & sub_type,
    size_t sub_size,
    int binary_idx,
    int field_idx)
{
	const auto source_content = read_source_content(plugin_idx, rec_type, record_id);
	if (source_content.empty())
	{
		m_log(
		    "[warning] copy_field: empty source content for " + rec_type + ":" + record_id + " in plugin " +
		    std::to_string(plugin_idx));
		return;
	}

	const auto merge_content = ensure_active_record(plugin_idx, rec_type, record_id, source_content);
	if (merge_content.empty())
	{
		m_log("[warning] copy_field: could not ensure active record for " + rec_type + ":" + record_id);
		return;
	}

	const auto result = merge_patch_ops_t::patch_field(
	    merge_content, source_content, rec_type, sub_type, sub_size, binary_idx, field_idx);
	if (!result.success)
	{
		m_log(
		    "[warning] copy_field: patch failed for " + sub_type + " field_idx=" + std::to_string(field_idx) +
		    " (binary_idx=" + std::to_string(binary_idx) + ") in " + rec_type + ":" + record_id);
		return;
	}

	m_session.scan().copy_record_to_active_raw(rec_type, record_id, result.content);
	m_log(
	    "[info] copied field " + result.description + " of " + sub_type + " from " +
	    m_session.scan().plugin_filename(plugin_idx) + " to active plugin (" + rec_type + ":" + record_id + ")");

	refresh_after_active_edit(rec_type, record_id);
	save_active_plugin();
}

void merge_controller_t::copy_bit(const copy_bit_params_t & params)
{
	const auto source_content = read_source_content(params.plugin_idx, params.rec_type, params.record_id);
	if (source_content.empty())
	{
		m_log("[warning] copy_bit: empty source content for " + params.rec_type + ":" + params.record_id);
		return;
	}

	const auto merge_content =
	    ensure_active_record(params.plugin_idx, params.rec_type, params.record_id, source_content);
	if (merge_content.empty())
	{
		m_log("[warning] copy_bit: could not ensure active record for " + params.rec_type + ":" + params.record_id);
		return;
	}

	const auto result = merge_patch_ops_t::patch_bit(merge_content, source_content, params.bit);
	if (!result.success)
	{
		m_log(
		    "[warning] copy_bit: patch failed for " + params.bit.sub_type + " bit=" +
		    std::to_string(params.bit.bit_index) + " in " + params.rec_type + ":" + params.record_id);
		return;
	}

	m_session.scan().copy_record_to_active_raw(params.rec_type, params.record_id, result.content);
	m_log(
	    "[info] copied bit " + result.description + " of " + params.bit.sub_type + " from " +
	    m_session.scan().plugin_filename(params.plugin_idx) + " to active plugin (" + params.rec_type + ":" +
	    params.record_id + ")");

	refresh_after_active_edit(params.rec_type, params.record_id);
	save_active_plugin();
}

void merge_controller_t::remove_sub_record(
    const std::string & rec_type,
    const std::string & record_id,
    int binary_idx,
    const std::string & removed_type)
{
	const auto * entry = m_session.scan().find(rec_type, record_id);
	if (!entry)
	{
		m_log("[warning] remove_sub_record: no conflict entry for " + rec_type + ":" + record_id);
		return;
	}

	std::string merge_content;
	for (const auto & version : entry->versions)
	{
		if (!m_session.scan().is_active_plugin(version.plugin_idx))
			continue;

		merge_content = m_session.scan().read_record_content(version.plugin_idx, version.record_index);
		break;
	}

	if (merge_content.empty())
	{
		m_log("[warning] remove_sub_record: no merge content for " + rec_type + ":" + record_id);
		return;
	}

	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	if (binary_idx >= static_cast<int>(merge_subs.size()))
	{
		m_log(
		    "[warning] remove_sub_record: binary_idx " + std::to_string(binary_idx) + " out of range (" +
		    std::to_string(merge_subs.size()) + ") in " + rec_type + ":" + record_id);
		return;
	}

	merge_subs.erase(merge_subs.begin() + binary_idx);

	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	m_session.scan().copy_record_to_active_raw(rec_type, record_id, patched);
	m_log("[info] removed " + removed_type + " from active plugin (" + rec_type + ":" + record_id + ")");

	refresh_after_active_edit(rec_type, record_id);
	save_active_plugin();
}

void merge_controller_t::remove_group(
    const std::string & rec_type,
    const std::string & record_id,
    view_tree_model_t::binary_range_t range)
{
	const auto * entry = m_session.scan().find(rec_type, record_id);
	if (!entry)
	{
		m_log("[warning] remove_group: no conflict entry for " + rec_type + ":" + record_id);
		return;
	}

	std::string merge_content;
	for (const auto & version : entry->versions)
	{
		if (!m_session.scan().is_active_plugin(version.plugin_idx))
			continue;

		merge_content = m_session.scan().read_record_content(version.plugin_idx, version.record_index);
		break;
	}

	if (merge_content.empty())
	{
		m_log("[warning] remove_group: no merge content for " + rec_type + ":" + record_id);
		return;
	}

	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	if (range.end_pos > static_cast<int>(merge_subs.size()))
	{
		m_log(
		    "[warning] remove_group: range end " + std::to_string(range.end_pos) + " out of range (" +
		    std::to_string(merge_subs.size()) + ") in " + rec_type + ":" + record_id);
		return;
	}

	merge_subs.erase(merge_subs.begin() + range.start, merge_subs.begin() + range.end_pos);

	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	m_session.scan().copy_record_to_active_raw(rec_type, record_id, patched);
	m_log("[info] removed group from active plugin (" + rec_type + ":" + record_id + ")");

	refresh_after_active_edit(rec_type, record_id);
	save_active_plugin();
}

void merge_controller_t::remove_record_from_active(const std::string & rec_type, const std::string & record_id)
{
	m_session.scan().remove_from_active(rec_type, record_id);
	m_session.scan().rebuild_conflicts();

	if (m_refresh)
		m_refresh();
	else
		m_nav_view.rebuild_preserving_state();

	save_active_plugin();
	m_log("[info] removed " + rec_type + ":" + record_id + " from active plugin");
}

bool merge_controller_t::is_active_locked(const merge_lock_t & lock) const
{
	return m_session.scan().has_active_lock(lock);
}

std::string merge_controller_t::capture_locked_content(const merge_lock_t & lock) const
{
	const auto * active_content = m_session.scan().find_active_content(lock.rec_type, lock.record_id);
	if (active_content == nullptr)
		return {};

	return *active_content;
}

void merge_controller_t::toggle_active_lock(const merge_lock_t & lock)
{
	if (m_session.scan().has_active_lock(lock))
	{
		m_session.scan().remove_active_lock(lock);
		m_log("[info] unlocked " + lock.rec_type + ":" + lock.record_id + " in merged patch");
	}
	else
	{
		auto stored = lock;
		stored.frozen_content = capture_locked_content(lock);
		if (stored.frozen_content.empty())
		{
			m_log("[warning] cannot lock " + lock.rec_type + ":" + lock.record_id + ": not in merged patch");
			return;
		}

		if (stored.scope == lock_scope_t::group)
			stored.group_members =
			    sub_record_merge_t::group_members_in_range(stored.frozen_content, stored.group_start, stored.group_end);

		m_session.scan().add_active_lock(stored);
		m_log("[info] locked " + lock.rec_type + ":" + lock.record_id + " in merged patch");
	}

	save_merged_patch_locks();

	if (m_lock_changed)
		m_lock_changed(lock.rec_type, lock.record_id);
}

void merge_controller_t::reapply_locks()
{
	for (const auto & lock : m_session.scan().active_locks())
	{
		const auto * current = m_session.scan().find_active_content(lock.rec_type, lock.record_id);
		const std::string merge_content = current ? *current : std::string {};

		patch_result_t result;

		switch (lock.scope)
		{
		case lock_scope_t::whole_record:
			m_session.scan().copy_record_to_active_raw(lock.rec_type, lock.record_id, lock.frozen_content);
			continue;

		case lock_scope_t::sub_record:
		{
			const auto frozen_subs = sub_record_merge_t::parse_sub_records(lock.frozen_content);
			const int frozen_idx =
			    sub_record_merge_t::find_by_type_and_occurrence(frozen_subs, lock.sub_type, lock.occurrence);
			if (frozen_idx < 0 || merge_content.empty())
				continue;

			result = merge_patch_ops_t::patch_sub_record(merge_content, lock.frozen_content, lock.sub_type, frozen_idx);
			break;
		}

		case lock_scope_t::field:
		{
			const auto frozen_subs = sub_record_merge_t::parse_sub_records(lock.frozen_content);
			const int frozen_idx =
			    sub_record_merge_t::find_by_type_and_occurrence(frozen_subs, lock.sub_type, lock.occurrence);
			if (frozen_idx < 0 || merge_content.empty())
				continue;

			result = merge_patch_ops_t::patch_field(
			    merge_content, lock.frozen_content, lock.rec_type, lock.sub_type, lock.sub_size, frozen_idx,
			    lock.field_index);
			break;
		}

		case lock_scope_t::bit:
		{
			const auto frozen_subs = sub_record_merge_t::parse_sub_records(lock.frozen_content);
			const int frozen_idx =
			    sub_record_merge_t::find_by_type_and_occurrence(frozen_subs, lock.sub_type, lock.occurrence);
			if (frozen_idx < 0 || merge_content.empty())
				continue;

			merge_patch_ops_t::bit_patch_params_t params;
			params.record_type = lock.rec_type;
			params.sub_type = lock.sub_type;
			params.sub_size = lock.sub_size;
			params.binary_idx = frozen_idx;
			params.field_idx = lock.field_index;
			params.bit_index = lock.bit_index;
			result = merge_patch_ops_t::patch_bit(merge_content, lock.frozen_content, params);
			break;
		}

		case lock_scope_t::group:
		{
			if (merge_content.empty())
				continue;

			result = merge_patch_ops_t::patch_group(merge_content, lock.frozen_content, lock.group_members);
			break;
		}
		}

		if (result.success)
			m_session.scan().copy_record_to_active_raw(lock.rec_type, lock.record_id, result.content);
	}
}

bool merge_controller_t::remove_record_from_plugin(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id)
{
	if (plugin_idx < 0 || m_session.scan().is_active_plugin(plugin_idx))
		return false;

	const auto * entry = m_session.scan().find(rec_type, record_id);
	if (!entry)
		return false;

	size_t record_index = 0;
	bool found = false;
	for (const auto & version : entry->versions)
	{
		if (version.plugin_idx != plugin_idx)
			continue;

		record_index = version.record_index;
		found = true;
		break;
	}

	if (!found)
		return false;

	const auto & plugin_filename = m_session.scan().plugin_filename(plugin_idx);

	m_session.scan().mutable_plugin(plugin_idx).remove_record(record_index);
	m_session.mark_plugin_dirty(plugin_idx);
	m_session.scan().rebuild_conflicts();

	if (m_refresh)
		m_refresh();
	else
		m_nav_view.rebuild_preserving_state();

	if (m_record_removal)
		m_record_removal({ plugin_filename, rec_type, record_id });

	m_log("[info] removed " + rec_type + ":" + record_id + " from " + plugin_filename);
	return true;
}

int merge_controller_t::create_merge_records()
{
	merge_config_t config;
	config.excluded_plugins = m_session.excluded_plugins();
	config.patch_plugins = m_session.patch_plugins();
	config.exclusion_pattern = m_settings.merge_exclusion_pattern();
	config.fog_fix_enabled = m_settings.merge_fog_fix_enabled();
	config.summon_fix_enabled = m_settings.merge_summon_fix_enabled();
	config.cell_name_fix_enabled = m_settings.merge_cell_name_fix_enabled();
	config.ignored_sub_records = parse_sub_record_rules(m_settings.sub_record_ignore_conflict());

	auto_merge_t merge(m_session.scan());
	merge.set_config(config);
	if (m_progress)
		merge.set_progress_callback(m_progress);
	const auto counters = merge.execute();

	reapply_locks();

	for (const auto & entry : merge.log_entries())
		m_log(entry.message);

	return counters.three_way + counters.lists + counters.dialogues + counters.fixes;
}

std::string merge_controller_t::resolve_active_output_path() const
{
	const auto output_dir = resolve_output_directory();
	if (output_dir.empty())
		return {};

	const int active_idx = m_session.scan().active_plugin_index();
	const std::string filename =
	    active_idx >= 0 ? m_session.scan().plugin_filename(active_idx) : std::string(merged_patch::filename);

	return QDir(QString::fromStdString(output_dir)).filePath(QString::fromStdString(filename)).toStdString();
}

std::string merge_controller_t::output_dir_relative_for_source() const
{
	if (m_session.load_source() == plugin_session_t::load_source_t::mo2_profile)
		return m_settings.output_dir_mo2();

	if (m_session.load_source() == plugin_session_t::load_source_t::openmw_cfg)
		return m_settings.output_dir_openmw();

	return m_settings.output_dir_folder();
}

std::string merge_controller_t::resolve_output_directory() const
{
	if (m_session.load_base_path().empty())
		return {};

	const auto base = QString::fromStdString(m_session.load_base_path());
	const auto relative = QString::fromStdString(output_dir_relative_for_source());

	if (relative.isEmpty())
		return QDir::cleanPath(base).toStdString();

	return QDir::cleanPath(QDir(base).filePath(relative)).toStdString();
}

void merge_controller_t::save_active_plugin()
{
	const auto output_path = resolve_active_output_path();
	if (output_path.empty())
	{
		m_log(
		    "[error] cannot save active plugin: output path is empty (load_base_path=" + m_session.load_base_path() +
		    ")");
		return;
	}

	auto output_dir = QDir(QFileInfo(QString::fromStdString(output_path)).absolutePath());
	output_dir.mkpath(".");

	const auto output_filename = std::filesystem::path(output_path).filename().string();
	const std::string description =
	    output_filename == merged_patch::filename ? "Auto-generated merged patch" : "Created with yEditor";
	const bool saved = save_active_to_file(output_path, "yEditor", description);
	if (saved)
		m_log(
		    "[info] saved " + output_path + " (" + std::to_string(m_session.scan().active_record_count()) + " records)");
	else
		m_log("[error] failed to save " + output_path);
}

bool merge_controller_t::save_plugin(int plugin_idx)
{
	auto & plugin = m_session.scan().mutable_plugin(plugin_idx);
	const auto & path = m_session.scan().plugin_path(plugin_idx);
	const bool written = binary_file_io::write_file(plugin.get_records(), path);
	if (!written)
	{
		m_log("[error] failed to save " + path);
		return false;
	}

	m_session.clear_plugin_dirty(plugin_idx);
	m_log("[info] saved " + path);
	return true;
}

void merge_controller_t::save_all_dirty()
{
	const auto dirty_copy = m_session.dirty_plugins();

	for (int plugin_idx = 0; plugin_idx < static_cast<int>(m_session.scan().plugin_count()); ++plugin_idx)
	{
		if (dirty_copy.count(m_session.scan().plugin_filename(plugin_idx)) == 0)
			continue;

		save_plugin(plugin_idx);
	}
}

bool merge_controller_t::save_active_to_file(
    const std::string & output_path,
    const std::string & author,
    const std::string & description)
{
	auto & scan = m_session.scan();
	auto & builder = m_session.patch_builder();

	if (!scan.has_active())
		return false;

	builder.clear();
	for (size_t i = 0; i < scan.active_record_count(); ++i)
	{
		if (scan.active_record_type(i) == "TES3")
			continue;

		builder.add_record_raw(scan.active_record_type(i), scan.active_record_id(i), scan.active_record_content(i));
	}

	const auto contributing = collect_contributing_plugins();
	const auto masters = build_master_list(contributing);
	const auto merge_filename = std::filesystem::path(output_path).filename().string();
	const bool tes3_is_new = (scan.find_active_content("TES3", merge_filename) == nullptr);

	const auto header_content =
	    patch_builder_t::build_tes3_header(author, description, builder.record_count(), masters);

	scan.copy_record_to_active_raw("TES3", merge_filename, header_content);

	const bool saved = builder.save(output_path, author, description, masters);

	if (saved && tes3_is_new)
	{
		scan.recompute_single_conflict("TES3", merge_filename);
		m_nav_view.rebuild_preserving_state();
	}

	return saved;
}

std::set<int> merge_controller_t::collect_contributing_plugins() const
{
	auto & scan = m_session.scan();
	std::set<int> contributing;

	for (const auto & entry : scan.entries())
	{
		bool in_merge = false;
		for (const auto & version : entry.versions)
		{
			if (scan.is_active_plugin(version.plugin_idx))
			{
				in_merge = true;
				break;
			}
		}

		if (!in_merge)
			continue;

		for (const auto & version : entry.versions)
		{
			if (!scan.is_active_plugin(version.plugin_idx))
				contributing.insert(version.plugin_idx);
		}
	}

	return contributing;
}

std::vector<patch_builder_t::master_entry_t> merge_controller_t::build_master_list(
    const std::set<int> & contributing) const
{
	auto & scan = m_session.scan();
	std::vector<patch_builder_t::master_entry_t> masters;

	for (int i = 0; i < static_cast<int>(scan.plugin_count()); ++i)
	{
		if (scan.is_active_plugin(i))
			continue;

		if (contributing.find(i) == contributing.end())
			continue;

		patch_builder_t::master_entry_t master;
		master.filename = scan.plugin_filename(i);

		try
		{
			master.file_size = std::filesystem::file_size(scan.plugin_path(i));
		}
		catch (...)
		{
			master.file_size = 0;
		}

		masters.push_back(std::move(master));
	}

	return masters;
}

void merge_controller_t::refresh_after_active_edit(const std::string & rec_type, const std::string & record_id)
{
	m_session.scan().recompute_single_conflict(rec_type, record_id);

	if (m_refresh)
	{
		m_refresh();
		return;
	}

	m_nav_view.refresh_colors();

	const auto * updated = m_session.scan().find(rec_type, record_id);
	if (!updated)
		return;

	m_record_view.display_record(m_session.scan(), *updated);
}

std::string merge_controller_t::read_source_content(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id)
{
	const auto * entry = m_session.scan().find(rec_type, record_id);
	if (!entry)
		return {};

	for (const auto & version : entry->versions)
	{
		if (version.plugin_idx == plugin_idx)
			return m_session.scan().read_record_content(plugin_idx, version.record_index);
	}

	return {};
}

std::string merge_controller_t::ensure_active_record(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & source_content)
{
	(void)plugin_idx;
	const auto * merge_content_ptr = m_session.scan().find_active_content(rec_type, record_id);
	if (merge_content_ptr)
		return *merge_content_ptr;

	const auto header_only = sub_record_merge_t::reconstruct_record(source_content, {});
	if (header_only.empty())
		return {};

	m_session.scan().copy_record_to_active_raw(rec_type, record_id, header_only);
	return header_only;
}

int merge_controller_t::find_merged_patch_index() const
{
	const auto & scan = m_session.scan();
	for (int i = 0; i < static_cast<int>(scan.plugin_count()); ++i)
	{
		if (scan.plugin_filename(i) == merged_patch::filename)
			return i;
	}

	return -1;
}

void merge_controller_t::sync_active_locks()
{
	const int active_idx = m_session.scan().active_plugin_index();
	if (active_idx >= 0 && m_session.scan().plugin_filename(active_idx) == merged_patch::filename)
		load_merged_patch_locks();
	else
		m_session.scan().set_active_locks({});
}

std::string merge_controller_t::merged_patch_locks_path() const
{
	const auto output_dir = resolve_output_directory();
	if (output_dir.empty())
		return {};

	const auto locks_name = std::string(merged_patch::filename) + std::string(merged_patch::locks_suffix);
	return QDir(QString::fromStdString(output_dir)).filePath(QString::fromStdString(locks_name)).toStdString();
}

void merge_controller_t::save_merged_patch_locks() const
{
	const auto path = merged_patch_locks_path();
	if (path.empty())
		return;

	const auto & locks = m_session.scan().active_locks();

	QFile::remove(QString::fromStdString(path));

	if (locks.empty())
		return;

	QSettings sidecar(QString::fromStdString(path), QSettings::IniFormat);
	sidecar.beginWriteArray("locks");
	for (int i = 0; i < static_cast<int>(locks.size()); ++i)
	{
		const auto & lock = locks[static_cast<size_t>(i)];
		sidecar.setArrayIndex(i);
		sidecar.setValue("rec_type", QString::fromStdString(lock.rec_type));
		sidecar.setValue("record_id", QString::fromStdString(lock.record_id));
		sidecar.setValue("scope", static_cast<int>(lock.scope));
		sidecar.setValue("sub_type", QString::fromStdString(lock.sub_type));
		sidecar.setValue("occurrence", lock.occurrence);
		sidecar.setValue("field_index", lock.field_index);
		sidecar.setValue("bit_index", lock.bit_index);
		sidecar.setValue("sub_size", static_cast<qulonglong>(lock.sub_size));
		sidecar.setValue("group_start", lock.group_start);
		sidecar.setValue("group_end", lock.group_end);
		sidecar.setValue(
		    "frozen",
		    QString::fromLatin1(
		        QByteArray(lock.frozen_content.data(), static_cast<int>(lock.frozen_content.size())).toBase64()));
	}

	sidecar.endArray();
}

void merge_controller_t::load_merged_patch_locks()
{
	const auto path = merged_patch_locks_path();
	if (path.empty() || !QFile::exists(QString::fromStdString(path)))
	{
		m_session.scan().set_active_locks({});
		return;
	}

	QSettings sidecar(QString::fromStdString(path), QSettings::IniFormat);
	std::vector<merge_lock_t> locks;

	const int size = sidecar.beginReadArray("locks");
	for (int i = 0; i < size; ++i)
	{
		sidecar.setArrayIndex(i);
		merge_lock_t lock;

		const int scope_value = sidecar.value("scope").toInt();
		if (!merge_lock_scope::scope_from_value(scope_value, lock.scope))
		{
			app_logger_t::add_log(
			    "[debug] skipping lock with invalid scope " + std::to_string(scope_value) + " in " + path + "\r\n",
			    true);
			continue;
		}

		lock.rec_type = sidecar.value("rec_type").toString().toStdString();
		lock.record_id = sidecar.value("record_id").toString().toStdString();
		lock.sub_type = sidecar.value("sub_type").toString().toStdString();
		lock.occurrence = sidecar.value("occurrence").toInt();
		lock.field_index = sidecar.value("field_index", -1).toInt();
		lock.bit_index = sidecar.value("bit_index", -1).toInt();
		lock.sub_size = static_cast<size_t>(sidecar.value("sub_size", 0).toULongLong());
		lock.group_start = sidecar.value("group_start", -1).toInt();
		lock.group_end = sidecar.value("group_end", -1).toInt();

		const auto decoded = QByteArray::fromBase64(sidecar.value("frozen").toString().toLatin1());
		lock.frozen_content.assign(decoded.constData(), static_cast<size_t>(decoded.size()));

		if (lock.scope == lock_scope_t::group)
			lock.group_members =
			    sub_record_merge_t::group_members_in_range(lock.frozen_content, lock.group_start, lock.group_end);

		locks.push_back(std::move(lock));
	}

	sidecar.endArray();
	m_session.scan().set_active_locks(locks);
}

int merge_controller_t::find_plugin_column(int plugin_idx) const
{
	const auto & indices = m_record_view.model()->column_plugin_indices();
	for (int column = 0; column < static_cast<int>(indices.size()); ++column)
	{
		if (indices[column] == plugin_idx)
			return column;
	}

	return -1;
}
