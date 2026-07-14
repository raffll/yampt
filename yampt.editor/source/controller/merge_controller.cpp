#include "merge_controller.hpp"
#include "../patcher/patch_builder.hpp"
#include "../session/plugin_session.hpp"
#include "../view/nav_tree_view.hpp"
#include "../view/record_view.hpp"
#include <scanner/auto_merge.hpp>
#include <scanner/merge_patch_ops.hpp>
#include <scanner/sub_record_merge.hpp>
#include <utility/record_behavior.hpp>
#include <filesystem>
#include <set>
#include <settings_store.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>

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

void merge_controller_t::create_merged_patch()
{
	if (m_session.scan().plugin_count() < 2)
	{
		m_log("Need at least 2 plugins loaded to create a merged patch");
		return;
	}

	if (!m_session.scan().has_merge())
		m_session.scan().set_merge_plugin("Merged Patch.esp");

	create_merge_records();
	m_session.scan().rebuild_conflicts();
	m_nav_view.rebuild_preserving_state();

	m_log("[info] merge record count: " + std::to_string(m_session.scan().merge_record_count()));
	save_merged_patch();
}

void merge_controller_t::save_plugin()
{
	if (!m_session.scan().has_merge())
	{
		m_log("[error] no merged patch exists");
		return;
	}

	if (m_session.scan().merge_record_count() == 0)
	{
		m_log("[error] merged patch is empty");
		return;
	}

	auto output_path = resolve_merge_output_path();
	if (output_path.empty())
	{
		const auto initial_dir = QString::fromStdString(m_settings.last_directory());
		const auto selected = QFileDialog::getSaveFileName(
		    nullptr, QCoreApplication::translate("yEditor", "Save Merge Plugin"), initial_dir, "ESP files (*.esp)");
		if (selected.isEmpty())
			return;

		output_path = selected.toStdString();
	}

	bool ok_author = false;
	auto author = QInputDialog::getText(nullptr, "Plugin Author", "Author:", QLineEdit::Normal, QString(), &ok_author);
	if (!ok_author)
		return;

	bool ok_desc = false;
	auto description =
	    QInputDialog::getText(nullptr, "Plugin Description", "Description:", QLineEdit::Normal, QString(), &ok_desc);
	if (!ok_desc)
		return;

	auto output_dir = QDir(QFileInfo(QString::fromStdString(output_path)).absolutePath());
	output_dir.mkpath(".");

	const bool result = save_merge_to_file(output_path, author.toStdString(), description.toStdString());
	if (result)
		m_log(
		    "[info] saved " + output_path + " (" + std::to_string(m_session.scan().merge_record_count()) + " records)");
	else
		m_log("[error] cannot write to " + output_path);
}

void merge_controller_t::load_existing_merged_patch()
{
	const auto path = resolve_merge_output_path();
	if (path.empty())
		return;

	if (!std::filesystem::exists(path))
	{
		m_log("[warning] merged patch not found: " + path);
		return;
	}

	if (m_session.scan().has_merge())
		return;

	auto merge_filename = std::filesystem::path(path).filename().string();

	for (int i = 0; i < static_cast<int>(m_session.scan().plugin_count()); ++i)
	{
		if (m_session.scan().plugin_filename(i) == merge_filename)
		{
			m_session.scan().set_merge_plugin_from_loaded(i);
			m_session.scan().rebuild_conflicts();
			m_log("Tagged existing plugin as merge: " + merge_filename);
			return;
		}
	}

	try
	{
		m_session.scan().load_plugin(path);
		const int loaded_idx = static_cast<int>(m_session.scan().plugin_count()) - 1;
		m_session.scan().set_merge_plugin_from_loaded(loaded_idx);
		m_session.scan().rebuild_conflicts();
		m_log("Loaded existing merged patch: " + path);
	}
	catch (const std::exception & error)
	{
		m_log("[error] cannot load merged patch: " + std::string(error.what()));
	}
}

void merge_controller_t::copy_whole_record(int plugin_idx, const std::string & rec_type, const std::string & record_id)
{
	const auto * entry = m_session.scan().find(rec_type, record_id);
	if (!entry)
		return;

	for (const auto & ver : entry->versions)
	{
		if (ver.plugin_idx != plugin_idx)
			continue;

		m_session.scan().copy_record_to_merge(plugin_idx, ver.record_index);
		break;
	}

	m_log("[info] copied record to merge (" + rec_type + ":" + record_id + ")");
	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
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
		return;

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
				selected_frmr = static_cast<uint32_t>(std::stoul(walk_node->values[clicked_col]));
				found_frmr = true;
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
	m_session.scan().copy_record_to_merge_raw(rec_type, record_id, result);
	m_log("[info] copied CELL record to merge (" + record_id + ")");

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
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
		return;

	const auto merge_content = ensure_merge_record(plugin_idx, rec_type, record_id, source_content);
	if (merge_content.empty())
		return;

	const auto result = merge_patch_ops_t::patch_sub_record(merge_content, source_content, sub_type, binary_idx);
	if (!result.success)
		return;

	m_session.scan().copy_record_to_merge_raw(rec_type, record_id, result.content);
	m_log(
	    "[info] copied " + sub_type + " from " + m_session.scan().plugin_filename(plugin_idx) + " to merge (" +
	    rec_type + ":" + record_id + ")");

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
}

void merge_controller_t::copy_group(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    int group_row_idx)
{
	const auto source_content = read_source_content(plugin_idx, rec_type, record_id);
	if (source_content.empty())
		return;

	const auto merge_content = ensure_merge_record(plugin_idx, rec_type, record_id, source_content);
	if (merge_content.empty())
		return;

	const int col = find_plugin_column(plugin_idx);
	const auto & visible = m_record_view.model()->rows();
	if (group_row_idx < 0 || group_row_idx >= static_cast<int>(visible.size()))
		return;

	const auto & group_row = visible[group_row_idx];

	if (col < 0 || col >= static_cast<int>(group_row.binary_ranges.size()))
		return;

	const auto & source_range = group_row.binary_ranges[col];
	if (source_range.start < 0)
	{
		m_log("[error] copy_group: no binary range for column " + std::to_string(col));
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
	m_session.scan().copy_record_to_merge_raw(rec_type, record_id, patched);
	m_log(
	    "[info] copied group \"" + group_row.label + "\" from " + m_session.scan().plugin_filename(plugin_idx) +
	    " to merge (" + rec_type + ":" + record_id + ")");

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
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
		return;

	const auto merge_content = ensure_merge_record(plugin_idx, rec_type, record_id, source_content);
	if (merge_content.empty())
		return;

	const auto result = merge_patch_ops_t::patch_field(
	    merge_content, source_content, rec_type, sub_type, sub_size, binary_idx, field_idx);
	if (!result.success)
		return;

	m_session.scan().copy_record_to_merge_raw(rec_type, record_id, result.content);
	m_log(
	    "[info] copied field " + result.description + " of " + sub_type + " from " +
	    m_session.scan().plugin_filename(plugin_idx) + " to merge (" + rec_type + ":" + record_id + ")");

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
}

void merge_controller_t::remove_sub_record(
    const std::string & rec_type,
    const std::string & record_id,
    int binary_idx,
    const std::string & removed_type)
{
	const auto * entry = m_session.scan().find(rec_type, record_id);
	if (!entry)
		return;

	std::string merge_content;
	for (const auto & ver : entry->versions)
	{
		if (!m_session.scan().is_merge_plugin(ver.plugin_idx))
			continue;

		merge_content = m_session.scan().read_record_content(ver.plugin_idx, ver.record_index);
		break;
	}

	if (merge_content.empty())
		return;

	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	if (binary_idx >= static_cast<int>(merge_subs.size()))
		return;

	merge_subs.erase(merge_subs.begin() + binary_idx);

	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	m_session.scan().copy_record_to_merge_raw(rec_type, record_id, patched);
	m_log("[info] removed " + removed_type + " from merge (" + rec_type + ":" + record_id + ")");

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
}

void merge_controller_t::remove_group(
    const std::string & rec_type,
    const std::string & record_id,
    view_tree_model_t::binary_range_t range)
{
	const auto * entry = m_session.scan().find(rec_type, record_id);
	if (!entry)
		return;

	std::string merge_content;
	for (const auto & ver : entry->versions)
	{
		if (!m_session.scan().is_merge_plugin(ver.plugin_idx))
			continue;

		merge_content = m_session.scan().read_record_content(ver.plugin_idx, ver.record_index);
		break;
	}

	if (merge_content.empty())
		return;

	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	if (range.end_pos > static_cast<int>(merge_subs.size()))
		return;

	merge_subs.erase(merge_subs.begin() + range.start, merge_subs.begin() + range.end_pos);

	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	m_session.scan().copy_record_to_merge_raw(rec_type, record_id, patched);
	m_log("[info] removed group from merge (" + rec_type + ":" + record_id + ")");

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
}

void merge_controller_t::remove_record_from_merge(const std::string & rec_type, const std::string & record_id)
{
	m_session.scan().remove_from_merge(rec_type, record_id);
	m_session.scan().rebuild_conflicts();
	m_nav_view.rebuild_preserving_state();
	save_merged_patch();
	m_log("Removed " + rec_type + ":" + record_id + " from merge");
}

int merge_controller_t::create_merge_records()
{
	auto pinned_records = m_session.scan().collect_pinned_records();

	merge_config_t config;
	config.excluded_plugins = m_session.excluded_plugins();
	config.patch_plugins = m_session.patch_plugins();
	config.exclusion_pattern = m_settings.merge_exclusion_pattern();
	config.fog_fix_enabled = m_settings.merge_fog_fix_enabled();
	config.summon_fix_enabled = m_settings.merge_summon_fix_enabled();
	config.cell_name_fix_enabled = m_settings.merge_cell_name_fix_enabled();

	auto_merge_t merge(m_session.scan());
	merge.set_config(config);
	const auto counters = merge.execute();

	m_session.scan().restore_pinned_records(pinned_records);

	for (const auto & entry : merge.log_entries())
		m_log(entry.message);

	return counters.three_way + counters.lists + counters.dialogues + counters.fixes;
}

std::string merge_controller_t::resolve_merge_output_path() const
{
	if (m_session.load_base_path().empty())
		return {};

	const auto base = QString::fromStdString(m_session.load_base_path());

	if (m_session.load_source() == plugin_session_t::load_source_t::mo2_profile)
	{
		const auto relative = QString::fromStdString(m_settings.merge_path_mo2());
		return QDir::cleanPath(base + "/" + relative).toStdString();
	}

	if (m_session.load_source() == plugin_session_t::load_source_t::openmw_cfg)
	{
		const auto relative = QString::fromStdString(m_settings.merge_path_openmw());
		return QDir::cleanPath(base + "/" + relative).toStdString();
	}

	const auto relative = QString::fromStdString(m_settings.merge_path_folder());
	return QDir::cleanPath(base + "/" + relative).toStdString();
}

std::string merge_controller_t::resolve_output_directory() const
{
	const auto merge_path = resolve_merge_output_path();
	if (merge_path.empty())
		return {};

	return std::filesystem::path(merge_path).parent_path().string();
}

void merge_controller_t::save_merged_patch()
{
	const auto output_path = resolve_merge_output_path();
	if (output_path.empty())
	{
		m_log(
		    "[error] cannot save merged patch: output path is empty (load_base_path=" + m_session.load_base_path() +
		    ")");
		return;
	}

	auto output_dir = QDir(QFileInfo(QString::fromStdString(output_path)).absolutePath());
	output_dir.mkpath(".");
	const bool saved = save_merge_to_file(output_path, "yEditor", "Auto-generated merged patch");
	if (saved)
		m_log(
		    "[info] saved " + output_path + " (" + std::to_string(m_session.scan().merge_record_count()) + " records)");
	else
		m_log("[error] failed to save " + output_path);
}

bool merge_controller_t::save_merge_to_file(
    const std::string & output_path,
    const std::string & author,
    const std::string & description)
{
	auto & scan = m_session.scan();
	auto & builder = m_session.patch_builder();

	if (!scan.has_merge())
		return false;

	if (scan.merge_record_count() == 0)
		return false;

	builder.clear();
	for (size_t i = 0; i < scan.merge_record_count(); ++i)
	{
		if (scan.merge_record_type(i) == "TES3")
			continue;

		builder.add_record_raw(scan.merge_record_type(i), scan.merge_record_id(i), scan.merge_record_content(i));
	}

	std::set<int> contributing_plugins;
	for (const auto & entry : scan.entries())
	{
		bool in_merge = false;
		for (const auto & ver : entry.versions)
		{
			if (scan.is_merge_plugin(ver.plugin_idx))
			{
				in_merge = true;
				break;
			}
		}

		if (!in_merge)
			continue;

		for (const auto & ver : entry.versions)
		{
			if (!scan.is_merge_plugin(ver.plugin_idx))
				contributing_plugins.insert(ver.plugin_idx);
		}
	}

	std::vector<patch_builder_t::master_entry_t> masters;
	for (int i = 0; i < static_cast<int>(scan.plugin_count()); ++i)
	{
		if (scan.is_merge_plugin(i))
			continue;

		if (contributing_plugins.find(i) == contributing_plugins.end())
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

	const auto merge_filename = std::filesystem::path(output_path).filename().string();
	const bool tes3_is_new = (scan.find_merge_content("TES3", merge_filename) == nullptr);

	const auto header_content =
	    patch_builder_t::build_tes3_header(author, description, builder.record_count(), masters);

	scan.copy_record_to_merge_raw("TES3", merge_filename, header_content);

	const bool saved = builder.save(output_path, author, description, masters);

	if (saved && tes3_is_new)
	{
		scan.recompute_single_conflict("TES3", merge_filename);
		m_nav_view.rebuild_preserving_state();
	}

	return saved;
}

void merge_controller_t::refresh_after_merge(const std::string & rec_type, const std::string & record_id)
{
	m_session.scan().recompute_single_conflict(rec_type, record_id);
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

	for (const auto & ver : entry->versions)
	{
		if (ver.plugin_idx == plugin_idx)
			return m_session.scan().read_record_content(plugin_idx, ver.record_index);
	}

	return {};
}

std::string merge_controller_t::ensure_merge_record(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & source_content)
{
	const auto * merge_content_ptr = m_session.scan().find_merge_content(rec_type, record_id);
	if (merge_content_ptr)
		return *merge_content_ptr;

	const auto header_only = sub_record_merge_t::reconstruct_record(source_content, {});
	m_session.scan().copy_record_to_merge_raw(rec_type, record_id, header_only);
	return header_only;
}

int merge_controller_t::find_plugin_column(int plugin_idx) const
{
	const auto & indices = m_record_view.model()->column_plugin_indices();
	for (int col = 0; col < static_cast<int>(indices.size()); ++col)
	{
		if (indices[col] == plugin_idx)
			return col;
	}

	return -1;
}
