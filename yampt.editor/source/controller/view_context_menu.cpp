#include <settings_store.hpp>
#include "view_context_menu.hpp"
#include "../session/merged_patch_name.hpp"
#include "../session/plugin_session.hpp"
#include "../view/nav_tree_view.hpp"
#include "../view/record_view.hpp"
#include "merge_controller.hpp"
#include <scanner/exclusion_resolver.hpp>
#include <scanner/record_conflict.hpp>
#include <utility/app_logger.hpp>
#include <utility/record_behavior.hpp>
#include <regex>
#include <set>
#include <string>
#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QMenu>
#include <QMessageBox>

static std::set<std::string> parse_ignore_rules(const std::string & serialized)
{
	std::set<std::string> rules;
	size_t start = 0;

	while (start < serialized.size())
	{
		auto comma = serialized.find(',', start);
		if (comma == std::string::npos)
			comma = serialized.size();

		auto token = serialized.substr(start, comma - start);
		auto trim_start = token.find_first_not_of(' ');
		if (trim_start != std::string::npos)
			rules.insert(token.substr(trim_start));

		start = comma + 1;
	}

	return rules;
}

static std::string serialize_ignore_rules(const std::set<std::string> & rules)
{
	std::string result;
	for (const auto & rule : rules)
	{
		if (!result.empty())
			result += ", ";

		result += rule;
	}

	return result;
}

static std::string anchored_exclusion_token(const std::string & record_id)
{
	return "^" + exclusion_resolver::regex_escape_literal(record_id) + "$";
}

static std::string append_exclusion_token(const std::string & pattern, const std::string & token)
{
	if (pattern.empty())
		return token;

	return pattern + "|" + token;
}

static std::string remove_exclusion_token(const std::string & pattern, const std::string & token)
{
	std::string result;
	size_t start = 0;

	while (start <= pattern.size())
	{
		const auto bar = pattern.find('|', start);
		const auto end = (bar == std::string::npos) ? pattern.size() : bar;
		const auto piece = pattern.substr(start, end - start);

		if (piece != token)
		{
			if (!result.empty())
				result += "|";

			result += piece;
		}

		if (bar == std::string::npos)
			break;

		start = bar + 1;
	}

	return result;
}

static bool pattern_compiles(const std::string & pattern)
{
	if (pattern.empty())
		return true;

	try
	{
		std::regex compiled(pattern, std::regex::icase);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

view_context_menu_t::view_context_menu_t(
    plugin_session_t & session,
    record_view_t & record_view,
    nav_tree_view_t & nav_view,
    merge_controller_t & merge_controller,
    settings_store_t & settings,
    settings_changed_fn on_settings_changed,
    unsaved_changed_fn on_unsaved_changed)
    : m_session(session)
    , m_record_view(record_view)
    , m_nav_view(nav_view)
    , m_merge(merge_controller)
    , m_settings(settings)
    , m_on_settings_changed(std::move(on_settings_changed))
    , m_on_unsaved_changed(std::move(on_unsaved_changed))
{}

void view_context_menu_t::show_nav_menu(const QPoint & global_pos, const nav_tree_model_t::node_info_t & info)
{
	if (info.plugin_idx < 0)
		return;

	const bool is_active = m_session.scan().is_active_plugin(info.plugin_idx);
	QMenu menu;

	if (!info.record_id.empty() && is_active)
	{
		if (m_session.scan().plugin_filename(info.plugin_idx) == merged_patch::filename)
		{
			merge_lock_t lock;
			lock.rec_type = info.rec_type;
			lock.record_id = info.record_id;
			lock.scope = lock_scope_t::whole_record;

			const bool locked = m_merge.is_active_locked(lock);
			const auto lock_label = locked ? QCoreApplication::translate("yEditor", "Unlock in Merged Patch")
			                               : QCoreApplication::translate("yEditor", "Lock in Merged Patch");
			menu.addAction(lock_label, [this, lock]() { m_merge.toggle_active_lock(lock); });

			menu.addSeparator();
		}

		menu.addAction(
		    QCoreApplication::translate("yEditor", "Remove Record from Active Plugin"),
		    [this, info]() { m_merge.remove_record_from_active(info.rec_type, info.record_id); });

		add_exclude_record_action(menu, info);
	}
	else if (!info.record_id.empty() && !is_active)
	{
		const bool record_in_active = m_session.scan().find_active_content(info.rec_type, info.record_id) != nullptr;
		auto * copy_action = menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Record to Active Plugin"),
		    [this, info]() { m_merge.copy_whole_record(info.plugin_idx, info.rec_type, info.record_id); });
		copy_action->setEnabled(m_session.scan().has_active() && !record_in_active);

		menu.addSeparator();

		auto * remove_action = menu.addAction(
		    QCoreApplication::translate("yEditor", "Remove Record from Plugin"),
		    [this, info]() { confirm_remove_record_from_plugin(info); });
		remove_action->setEnabled(m_record_view.model()->is_editing_enabled());

		add_exclude_record_action(menu, info);
	}
	else if (info.rec_type.empty() && info.record_id.empty())
	{
		build_source_file_menu(menu, info);
	}

	if (menu.actions().isEmpty())
		return;

	menu.exec(global_pos);
}

void view_context_menu_t::build_source_file_menu(QMenu & menu, const nav_tree_model_t::node_info_t & info)
{
	const auto & filename = m_session.scan().plugin_filename(info.plugin_idx);
	const bool excluded = m_session.excluded_plugins().count(filename) > 0;
	const bool is_patch = m_session.patch_plugins().count(filename) > 0;
	const bool is_active = m_session.scan().is_active_plugin(info.plugin_idx);

	auto * set_active_action = menu.addAction(
	    QCoreApplication::translate("yEditor", "Set as Active Plugin"),
	    [this, info]() { m_merge.set_active_plugin(info.plugin_idx); });
	set_active_action->setToolTip(
	    QCoreApplication::translate("yEditor", "Make this the plugin that receives copied records"));
	set_active_action->setEnabled(!is_active);

	menu.addSeparator();

	auto * save_action = menu.addAction(
	    QCoreApplication::translate("yEditor", "Save"),
	    [this, info]()
	{
		if (m_merge.save_plugin(info.plugin_idx))
			m_nav_view.notify_plugin_changed(info.plugin_idx);

		if (m_on_unsaved_changed)
			m_on_unsaved_changed(m_session.has_any_unsaved());
	});
	save_action->setToolTip(QCoreApplication::translate("yEditor", "Write in-memory changes to the plugin file"));
	save_action->setEnabled(m_session.is_plugin_dirty(info.plugin_idx));

	menu.addSeparator();

	menu.addAction(
	    excluded ? QCoreApplication::translate("yEditor", "Include in Merged Patch")
	             : QCoreApplication::translate("yEditor", "Exclude from Merged Patch"),
	    [this, info, filename, excluded]()
	{
		auto excluded_copy = m_session.excluded_plugins();
		if (excluded)
		{
			excluded_copy.erase(filename);
		}
		else
		{
			excluded_copy.insert(filename);

			auto patch_copy = m_session.patch_plugins();
			if (patch_copy.erase(filename) > 0)
				m_session.set_patch_plugins(patch_copy);
		}

		m_session.set_excluded_plugins(excluded_copy);
		m_session.save_session_state(QDir(settings_store_t::settings_dir()).filePath("yEditor.ini"));
		m_nav_view.notify_plugin_changed(info.plugin_idx);
	});

	menu.addAction(
	    is_patch ? QCoreApplication::translate("yEditor", "Unmark as Guard Patch")
	             : QCoreApplication::translate("yEditor", "Mark as Guard Patch"),
	    [this, info, filename, is_patch]()
	{
		auto patch_copy = m_session.patch_plugins();
		if (is_patch)
		{
			patch_copy.erase(filename);
		}
		else
		{
			patch_copy.insert(filename);

			auto excluded_copy = m_session.excluded_plugins();
			if (excluded_copy.erase(filename) > 0)
				m_session.set_excluded_plugins(excluded_copy);
		}

		m_session.set_patch_plugins(patch_copy);
		m_session.save_session_state(QDir(settings_store_t::settings_dir()).filePath("yEditor.ini"));
		m_nav_view.notify_plugin_changed(info.plugin_idx);
	});
}

void view_context_menu_t::add_exclude_record_action(QMenu & menu, const nav_tree_model_t::node_info_t & info)
{
	const auto current_pattern = m_settings.merge_exclusion_pattern();

	exclusion_resolver_t resolver;
	resolver.set_pattern(current_pattern);
	const bool excluded = resolver.is_record_excluded(info.rec_type, info.record_id);

	const auto token = anchored_exclusion_token(info.record_id);
	const auto has_token = remove_exclusion_token(current_pattern, token) != current_pattern;

	menu.addSeparator();

	if (!excluded)
	{
		const auto next_pattern = append_exclusion_token(current_pattern, token);
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Exclude Record from Merged Patch"),
		    [this, info, next_pattern]() { apply_record_exclusion_pattern(info, next_pattern); });

		return;
	}

	if (!has_token)
	{
		auto * covered_action =
		    menu.addAction(QCoreApplication::translate("yEditor", "Include Record in Merged Patch"));
		covered_action->setEnabled(false);
		covered_action->setToolTip(
		    QCoreApplication::translate("yEditor", "This record is covered by a custom exclusion pattern"));

		return;
	}

	const auto next_pattern = remove_exclusion_token(current_pattern, token);
	menu.addAction(
	    QCoreApplication::translate("yEditor", "Include Record in Merged Patch"),
	    [this, info, next_pattern]() { apply_record_exclusion_pattern(info, next_pattern); });
}

void view_context_menu_t::apply_record_exclusion_pattern(
    const nav_tree_model_t::node_info_t & info,
    const std::string & pattern)
{
	if (!pattern_compiles(pattern))
	{
		app_logger_t::add_log("[warning] invalid exclusion pattern, keeping previous: " + pattern + "\r\n");

		return;
	}

	m_settings.set_merge_exclusion_pattern(pattern);
	m_session.save_session_state(QDir(settings_store_t::settings_dir()).filePath("yEditor.ini"));
	m_nav_view.notify_record_changed(info.rec_type, info.record_id);
}

void view_context_menu_t::confirm_remove_record_from_plugin(const nav_tree_model_t::node_info_t & info)
{
	const auto & filename = m_session.scan().plugin_filename(info.plugin_idx);

	const auto title = QCoreApplication::translate("yEditor", "Remove Record");
	const auto message = QCoreApplication::translate(
	                         "yEditor",
	                         "Remove record %1:%2 from \"%3\"?\n\nThis cannot be undone. The record is dropped from the "
	                         "plugin in memory and disappears from the file when you save it.")
	                         .arg(QString::fromStdString(info.rec_type))
	                         .arg(QString::fromStdString(info.record_id))
	                         .arg(QString::fromStdString(filename));

	const auto choice =
	    QMessageBox::question(nullptr, title, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (choice != QMessageBox::Yes)
		return;

	if (!m_merge.remove_record_from_plugin(info.plugin_idx, info.rec_type, info.record_id))
		return;

	if (m_on_unsaved_changed)
		m_on_unsaved_changed(m_session.has_any_unsaved());
}

void view_context_menu_t::show_view_menu(const QPoint & global_pos, const QModelIndex & index)
{
	if (!index.isValid())
		return;

	const auto * node = m_record_view.model()->node_from_index(index);
	if (!node)
		return;

	const auto & row = *node;
	const auto & rec_type = m_record_view.model()->record_type();
	const auto & record_id = m_record_view.model()->record_id();
	const bool is_field_row = index.parent().isValid();
	const int parent_row_idx = is_field_row ? index.parent().row() : index.row();

	const auto & visible = m_record_view.model()->rows();
	if (!is_field_row && (parent_row_idx < 0 || parent_row_idx >= static_cast<int>(visible.size())))
		return;

	const int col = index.column() - 1;
	const bool has_valid_column =
	    col >= 0 && col < static_cast<int>(m_record_view.model()->column_plugin_indices().size());

	const int plugin_idx = has_valid_column ? m_record_view.model()->column_plugin_indices()[col] : -1;

	const int bin_idx = (has_valid_column && col < static_cast<int>(row.binary_ranges.size()))
	                        ? row.binary_ranges[col].start
	                        : -1;

	const auto kind = [&]() -> row_kind_t
	{
		if (is_field_row && row.type.empty())
			return row_kind_t::field_of_schema;

		if (row.type.empty())
			return row_kind_t::other;

		if (is_field_row && !row.children.empty() && row.size == 0)
			return row_kind_t::field_of_group;

		if (is_field_row)
			return row_kind_t::field_of_schema;

		if (!row.children.empty() && row.size == 0)
			return row_kind_t::group;

		if (!row.children.empty() && row.size > 0 && bin_idx >= 0)
			return row_kind_t::schema_record;

		if (row.children.empty() && bin_idx >= 0)
			return row_kind_t::sub_record;

		return row_kind_t::other;
	}();

	view_menu_context_t context { index, row, rec_type, record_id, plugin_idx, col, bin_idx, parent_row_idx, kind };
	QMenu menu;

	if (has_valid_column && m_session.scan().has_active())
	{
		const bool is_on_active = m_session.scan().is_active_plugin(plugin_idx);
		const bool is_on_merged_patch = m_session.scan().plugin_filename(plugin_idx) == merged_patch::filename;
		const bool record_in_active = m_session.scan().find_active_content(rec_type, record_id) != nullptr;

		if (is_on_active && is_on_merged_patch)
			build_lock_menu(menu, context);
		else if (!record_in_active)
			build_copy_to_merge_menu(menu, context);
		else
			build_source_copy_menu(menu, context);
	}

	if (kind == row_kind_t::sub_record || kind == row_kind_t::schema_record)
		build_sub_record_ignore_menu(menu, context);

	if (has_valid_column && m_session.scan().has_active() && m_session.scan().is_active_plugin(plugin_idx))
		build_merge_remove_menu(menu, context);

	if (menu.actions().isEmpty())
		return;

	menu.exec(global_pos);
}

void view_context_menu_t::build_sub_record_ignore_menu(QMenu & menu, const view_menu_context_t & context)
{
	const auto rule = context.rec_type + ":" + context.row.type;
	const auto wildcard = context.rec_type + ":*";
	const auto rules = parse_ignore_rules(m_settings.sub_record_ignore_conflict());

	const bool excluded_by_rule = rules.count(rule) > 0;
	const bool excluded_by_wildcard = rules.count(wildcard) > 0;
	const bool already_excluded = excluded_by_rule || excluded_by_wildcard;

	if (!menu.actions().isEmpty())
		menu.addSeparator();

	const auto label = already_excluded ? QCoreApplication::translate("yEditor", "Include Sub-Record \"%1\"")
	                                     : QCoreApplication::translate("yEditor", "Exclude Sub-Record \"%1\"");

	auto * action = menu.addAction(
	    label.arg(QString::fromStdString(rule)),
	    [this, rule, excluded_by_rule]() { toggle_ignore_rule(rule, excluded_by_rule); });

	if (excluded_by_wildcard && !excluded_by_rule)
		action->setEnabled(false);
}

void view_context_menu_t::toggle_ignore_rule(const std::string & rule, bool remove_rule)
{
	auto rules = parse_ignore_rules(m_settings.sub_record_ignore_conflict());

	if (remove_rule)
		rules.erase(rule);
	else
		rules.insert(rule);

	m_settings.set_sub_record_ignore_conflict(serialize_ignore_rules(rules));

	if (m_on_settings_changed)
		m_on_settings_changed();
}

void view_context_menu_t::build_copy_to_merge_menu(QMenu & menu, const view_menu_context_t & context)
{
	const auto * behavior = find_record_behavior(context.rec_type);

	if (behavior->copy_strategy == copy_strategy_t::header_and_selected_group)
	{
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Record to Active Plugin"),
		    [this, &context]()
		{
			m_merge.copy_cell_record(
			    context.plugin_idx, context.rec_type, context.record_id, context.index, context.col);
		});
	}
	else
	{
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Record to Active Plugin"),
		    [this, &context]() { m_merge.copy_whole_record(context.plugin_idx, context.rec_type, context.record_id); });
	}
}

field_binary_resolver::resolved_field_t view_context_menu_t::resolve_schema_field(
    const view_menu_context_t & context) const
{
	std::vector<const view_tree_model_t::view_node_t *> ancestors;
	QModelIndex ancestor_index = context.index.parent();

	while (ancestor_index.isValid())
	{
		ancestors.push_back(m_record_view.model()->node_from_index(ancestor_index));
		ancestor_index = ancestor_index.parent();
	}

	return field_binary_resolver::resolve(ancestors, context.col, context.row.schema_field_index);
}

field_binary_resolver::resolved_bit_t view_context_menu_t::resolve_schema_bit(
    const view_menu_context_t & context) const
{
	std::vector<const view_tree_model_t::view_node_t *> ancestors;
	QModelIndex ancestor_index = context.index.parent();

	while (ancestor_index.isValid())
	{
		ancestors.push_back(m_record_view.model()->node_from_index(ancestor_index));
		ancestor_index = ancestor_index.parent();
	}

	return field_binary_resolver::resolve_bit(
	    ancestors, context.col, context.row.schema_field_index, context.row.bit_index);
}

void view_context_menu_t::add_copy_bit_action(QMenu & menu, const view_menu_context_t & context)
{
	const auto resolved = resolve_schema_bit(context);
	if (!resolved.found)
		return;

	merge_controller_t::copy_bit_params_t params;
	params.plugin_idx = context.plugin_idx;
	params.rec_type = context.rec_type;
	params.record_id = context.record_id;
	params.bit.record_type = context.rec_type;
	params.bit.sub_type = resolved.sub_type;
	params.bit.sub_size = resolved.sub_size;
	params.bit.binary_idx = resolved.binary_index;
	params.bit.field_idx = resolved.field_index;
	params.bit.bit_index = resolved.bit_index;

	menu.addAction(
	    QCoreApplication::translate("yEditor", "Copy Bit to Active Plugin"),
	    [this, params]() { m_merge.copy_bit(params); });
}

void view_context_menu_t::build_source_copy_menu(QMenu & menu, const view_menu_context_t & context)
{
	switch (context.kind)
	{
	case row_kind_t::sub_record:
	case row_kind_t::schema_record:
	{
		const auto sub_type = context.row.type;
		const auto bin_idx = context.bin_idx;
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Sub-Record to Active Plugin"),
		    [this, &context, sub_type, bin_idx]()
		{ m_merge.copy_sub_record(context.plugin_idx, context.rec_type, context.record_id, sub_type, bin_idx); });
		break;
	}

	case row_kind_t::group:
	{
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Group to Active Plugin"),
		    [this, &context]()
		{ m_merge.copy_group(context.plugin_idx, context.rec_type, context.record_id, context.parent_row_idx); });
		break;
	}

	case row_kind_t::field_of_schema:
	{
		if (context.row.bit_index >= 0)
		{
			add_copy_bit_action(menu, context);
			break;
		}

		const auto resolved = resolve_schema_field(context);
		if (!resolved.found)
			break;

		const auto sub_type = resolved.sub_type;
		const auto sub_size = resolved.sub_size;
		const int field_bin = resolved.binary_index;
		const int child_field_idx = context.row.schema_field_index;

		menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Field to Active Plugin"),
		    [this, &context, sub_type, sub_size, field_bin, child_field_idx]()
		{
			m_merge.copy_field(
			    context.plugin_idx,
			    context.rec_type,
			    context.record_id,
			    sub_type,
			    sub_size,
			    field_bin,
			    child_field_idx);
		});
		break;
	}

	case row_kind_t::field_of_group:
	{
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Group to Active Plugin"),
		    [this, &context]()
		{ m_merge.copy_group(context.plugin_idx, context.rec_type, context.record_id, context.parent_row_idx); });
		break;
	}

	case row_kind_t::other:
		break;
	}
}

void view_context_menu_t::build_merge_remove_menu(QMenu & menu, const view_menu_context_t & context)
{
	const auto & visible = m_record_view.model()->rows();

	switch (context.kind)
	{
	case row_kind_t::sub_record:
	case row_kind_t::schema_record:
	{
		if (context.bin_idx >= 0)
		{
			const auto removed_type = context.row.type;
			const auto bin_idx = context.bin_idx;

			if (!menu.actions().isEmpty())
				menu.addSeparator();

			menu.addAction(
			    QCoreApplication::translate("yEditor", "Remove Sub-Record from Active Plugin"),
			    [this, &context, bin_idx, removed_type]()
			{ m_merge.remove_sub_record(context.rec_type, context.record_id, bin_idx, removed_type); });
		}

		break;
	}

	case row_kind_t::group:
	case row_kind_t::field_of_group:
	{
		if (context.kind == row_kind_t::field_of_group &&
		    (context.parent_row_idx < 0 || context.parent_row_idx >= static_cast<int>(visible.size())))
			break;

		const auto & target_row =
		    (context.kind == row_kind_t::field_of_group) ? visible[context.parent_row_idx] : context.row;

		if (context.col < 0 || context.col >= static_cast<int>(target_row.binary_ranges.size()))
			break;

		auto merge_range = target_row.binary_ranges[context.col];

		if (merge_range.start >= 0)
		{
			if (!menu.actions().isEmpty())
				menu.addSeparator();

			menu.addAction(
			    QCoreApplication::translate("yEditor", "Remove Group from Active Plugin"),
			    [this, &context, merge_range]()
			{ m_merge.remove_group(context.rec_type, context.record_id, merge_range); });
		}

		break;
	}

	case row_kind_t::field_of_schema:
	{
		const auto resolved = resolve_schema_field(context);
		if (!resolved.found)
			break;

		const int merge_bin = resolved.binary_index;
		const auto removed_type = resolved.sub_type;

		if (!menu.actions().isEmpty())
			menu.addSeparator();

		menu.addAction(
		    QCoreApplication::translate("yEditor", "Remove Sub-Record from Active Plugin"),
		    [this, &context, merge_bin, removed_type]()
		{ m_merge.remove_sub_record(context.rec_type, context.record_id, merge_bin, removed_type); });

		break;
	}

	case row_kind_t::other:
		break;
	}
}

merge_lock_t view_context_menu_t::build_lock_for(const view_menu_context_t & context) const
{
	merge_lock_t lock;
	lock.rec_type = context.rec_type;
	lock.record_id = context.record_id;

	switch (context.kind)
	{
	case row_kind_t::sub_record:
	case row_kind_t::schema_record:
		lock.scope = lock_scope_t::sub_record;
		lock.sub_type = context.row.type;
		lock.occurrence = context.row.occurrence;
		lock.sub_size = context.row.size;
		break;

	case row_kind_t::field_of_schema:
	{
		if (context.row.bit_index >= 0)
		{
			const auto resolved = resolve_schema_bit(context);
			lock.scope = lock_scope_t::bit;
			lock.sub_type = resolved.sub_type;
			lock.sub_size = resolved.sub_size;
			lock.field_index = resolved.field_index;
			lock.bit_index = resolved.bit_index;
			lock.occurrence = resolved.occurrence;
			break;
		}

		const auto resolved = resolve_schema_field(context);
		lock.scope = lock_scope_t::field;
		lock.sub_type = resolved.sub_type;
		lock.sub_size = resolved.sub_size;
		lock.field_index = context.row.schema_field_index;
		lock.occurrence = resolved.occurrence;
		break;
	}

	case row_kind_t::group:
	case row_kind_t::field_of_group:
	{
		lock.scope = lock_scope_t::group;

		const auto & visible = m_record_view.model()->rows();
		if (context.parent_row_idx < 0 || context.parent_row_idx >= static_cast<int>(visible.size()))
			break;

		const auto & group_row = visible[context.parent_row_idx];
		if (context.col < 0 || context.col >= static_cast<int>(group_row.binary_ranges.size()))
			break;

		const auto & range = group_row.binary_ranges[context.col];
		if (range.start < 0)
			break;

		lock.group_start = range.start;
		lock.group_end = range.end_pos;
		break;
	}

	case row_kind_t::other:
		lock.scope = lock_scope_t::whole_record;
		break;
	}

	return lock;
}

void view_context_menu_t::build_lock_menu(QMenu & menu, const view_menu_context_t & context)
{
	const auto lock = build_lock_for(context);
	const bool needs_sub_type =
	    lock.scope == lock_scope_t::sub_record || lock.scope == lock_scope_t::field || lock.scope == lock_scope_t::bit;
	const bool invalid_sub_type = needs_sub_type && lock.sub_type.empty();
	const bool invalid_group = lock.scope == lock_scope_t::group && lock.group_start < 0;
	const bool can_lock = !invalid_sub_type && !invalid_group;

	const bool locked = can_lock && m_merge.is_active_locked(lock);

	if (!menu.actions().isEmpty())
		menu.addSeparator();

	const auto label = locked ? QCoreApplication::translate("yEditor", "Unlock in Merged Patch")
	                           : QCoreApplication::translate("yEditor", "Lock in Merged Patch");

	auto * action = can_lock ? menu.addAction(label, [this, lock]() { m_merge.toggle_active_lock(lock); })
	                         : menu.addAction(label);
	action->setEnabled(can_lock);
}
