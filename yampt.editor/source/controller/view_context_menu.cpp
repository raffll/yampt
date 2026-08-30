#include <settings_store.hpp>
#include "view_context_menu.hpp"
#include "../session/plugin_session.hpp"
#include "../view/nav_tree_view.hpp"
#include "../view/record_view.hpp"
#include "merge_controller.hpp"
#include <scanner/record_conflict.hpp>
#include <utility/record_behavior.hpp>
#include <QCoreApplication>
#include <QMenu>

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

	const bool is_merge = m_session.scan().is_merge_plugin(info.plugin_idx);
	QMenu menu;

	if (!info.record_id.empty() && is_merge)
	{
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Remove"),
		    [this, info]() { m_merge.remove_record_from_merge(info.rec_type, info.record_id); });
	}
	else if (info.rec_type.empty() && info.record_id.empty() && !is_merge)
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

	menu.addAction(
	    excluded ? QCoreApplication::translate("yEditor", "Include in Merged Patch")
	             : QCoreApplication::translate("yEditor", "Exclude from Merged Patch"),
	    [this, filename, excluded]()
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
		m_session.save_session_state(settings_store_t::settings_dir() + "yEditor.ini");
		m_nav_view.rebuild_preserving_state();
	});

	menu.addAction(
	    is_patch ? QCoreApplication::translate("yEditor", "Unmark as Guard Patch")
	             : QCoreApplication::translate("yEditor", "Mark as Guard Patch"),
	    [this, filename, is_patch]()
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
		m_session.save_session_state(settings_store_t::settings_dir() + "yEditor.ini");
		m_nav_view.rebuild_preserving_state();
	});

	auto * save_action = menu.addAction(
	    QCoreApplication::translate("yEditor", "Save"),
	    [this, info]()
	{
		if (m_merge.save_plugin(info.plugin_idx))
			m_nav_view.rebuild_preserving_state();

		if (m_on_unsaved_changed)
			m_on_unsaved_changed(m_session.has_any_unsaved());
	});
	save_action->setToolTip(QCoreApplication::translate("yEditor", "Write in-memory changes to the plugin file"));
	save_action->setEnabled(m_session.is_plugin_dirty(info.plugin_idx));
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
	if (parent_row_idx < 0 || parent_row_idx >= static_cast<int>(visible.size()))
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

	if (has_valid_column && m_session.scan().has_merge())
	{
		const bool is_on_merge = m_session.scan().is_merge_plugin(plugin_idx);
		const bool record_in_merge = m_session.scan().find_merge_content(rec_type, record_id) != nullptr;

		if (is_on_merge)
			build_merge_remove_menu(menu, context);
		else if (!record_in_merge)
			build_copy_to_merge_menu(menu, context);
		else
			build_source_copy_menu(menu, context);
	}

	if (kind == row_kind_t::sub_record || kind == row_kind_t::schema_record)
	{
		const auto sub_type = row.type;
		const auto rule = rec_type + ":" + sub_type;

		if (!menu.actions().isEmpty())
			menu.addSeparator();

		menu.addAction(
		    QCoreApplication::translate("yEditor", "Exclude Sub-Record \"%1\"").arg(QString::fromStdString(rule)),
		    [this, rule]()
		{
			auto current = m_settings.sub_record_ignore_conflict();

			std::set<std::string> existing;
			size_t start = 0;
			while (start < current.size())
			{
				auto comma = current.find(',', start);
				if (comma == std::string::npos)
					comma = current.size();

				auto token = current.substr(start, comma - start);
				auto trim_start = token.find_first_not_of(" ");
				if (trim_start != std::string::npos)
					existing.insert(token.substr(trim_start));

				start = comma + 1;
			}

			if (existing.count(rule))
				return;

			if (!current.empty())
				current += ", ";

			current += rule;
			m_settings.set_sub_record_ignore_conflict(current);

			if (m_on_settings_changed)
				m_on_settings_changed();
		});
	}

	if (menu.actions().isEmpty())
		return;

	menu.exec(global_pos);
}

void view_context_menu_t::build_copy_to_merge_menu(QMenu & menu, const view_menu_context_t & context)
{
	const auto * behavior = find_record_behavior(context.rec_type);

	if (behavior->copy_strategy == copy_strategy_t::header_and_selected_group)
	{
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Record to Merged Patch"),
		    [this, &context]()
		{
			m_merge.copy_cell_record(
			    context.plugin_idx, context.rec_type, context.record_id, context.index, context.col);
		});
	}
	else
	{
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Record to Merged Patch"),
		    [this, &context]() { m_merge.copy_whole_record(context.plugin_idx, context.rec_type, context.record_id); });
	}
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
		    QCoreApplication::translate("yEditor", "Copy Sub-Record to Merged Patch"),
		    [this, &context, sub_type, bin_idx]()
		{ m_merge.copy_sub_record(context.plugin_idx, context.rec_type, context.record_id, sub_type, bin_idx); });
		break;
	}

	case row_kind_t::group:
	{
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Group to Merged Patch"),
		    [this, &context]()
		{ m_merge.copy_group(context.plugin_idx, context.rec_type, context.record_id, context.parent_row_idx); });
		break;
	}

	case row_kind_t::field_of_schema:
	{
		QModelIndex sub_record_index = context.index.parent();
		const auto * sub_record_node = m_record_view.model()->node_from_index(sub_record_index);

		while (sub_record_node && sub_record_node->type.empty() && sub_record_index.parent().isValid())
		{
			sub_record_index = sub_record_index.parent();
			sub_record_node = m_record_view.model()->node_from_index(sub_record_index);
		}

		if (!sub_record_node || sub_record_node->type.empty())
			break;

		if (context.row.schema_field_index < 0)
			break;

		const auto sub_type = sub_record_node->type;
		const auto sub_size = sub_record_node->size;
		const int field_bin =
		    (context.col >= 0 && context.col < static_cast<int>(sub_record_node->binary_ranges.size()))
		        ? sub_record_node->binary_ranges[context.col].start
		        : -1;
		const int child_field_idx = context.row.schema_field_index;

		menu.addAction(
		    QCoreApplication::translate("yEditor", "Copy Field to Merged Patch"),
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
		    QCoreApplication::translate("yEditor", "Copy Group to Merged Patch"),
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
			menu.addAction(
			    QCoreApplication::translate("yEditor", "Remove Sub-Record"),
			    [this, &context, bin_idx, removed_type]()
			{ m_merge.remove_sub_record(context.rec_type, context.record_id, bin_idx, removed_type); });
		}

		break;
	}

	case row_kind_t::group:
	case row_kind_t::field_of_group:
	{
		const auto & target_row =
		    (context.kind == row_kind_t::field_of_group) ? visible[context.parent_row_idx] : context.row;

		if (context.col < 0 || context.col >= static_cast<int>(target_row.binary_ranges.size()))
			break;

		auto merge_range = target_row.binary_ranges[context.col];

		if (merge_range.start >= 0)
		{
			menu.addAction(
			    QCoreApplication::translate("yEditor", "Remove Group"),
			    [this, &context, merge_range]()
			{ m_merge.remove_group(context.rec_type, context.record_id, merge_range); });
		}

		break;
	}

	case row_kind_t::field_of_schema:
	{
		QModelIndex sub_record_index = context.index.parent();
		const auto * sub_record_node = m_record_view.model()->node_from_index(sub_record_index);

		while (sub_record_node && sub_record_node->type.empty() && sub_record_index.parent().isValid())
		{
			sub_record_index = sub_record_index.parent();
			sub_record_node = m_record_view.model()->node_from_index(sub_record_index);
		}

		if (!sub_record_node || sub_record_node->type.empty())
			break;

		const int merge_bin =
		    (context.col >= 0 && context.col < static_cast<int>(sub_record_node->binary_ranges.size()))
		        ? sub_record_node->binary_ranges[context.col].start
		        : -1;

		if (merge_bin >= 0)
		{
			const auto removed_type = sub_record_node->type;
			menu.addAction(
			    QCoreApplication::translate("yEditor", "Remove Sub-Record"),
			    [this, &context, merge_bin, removed_type]()
			{ m_merge.remove_sub_record(context.rec_type, context.record_id, merge_bin, removed_type); });
		}

		break;
	}

	case row_kind_t::other:
		break;
	}
}
