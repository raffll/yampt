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
    merge_controller_t & merge_controller)
    : m_session(session)
    , m_record_view(record_view)
    , m_nav_view(nav_view)
    , m_merge(merge_controller)
{}

void view_context_menu_t::show_nav_menu(const QPoint & global_pos, const nav_tree_model_t::node_info_t & info)
{
	if (info.plugin_idx < 0)
		return;

	enum class node_kind_t
	{
		record_on_merge,
		file_on_source,
		other
	};

	const bool is_merge = m_session.scan().is_merge_plugin(info.plugin_idx);

	const auto node_kind = [&]() -> node_kind_t
	{
		if (!info.record_id.empty() && is_merge)
			return node_kind_t::record_on_merge;

		if (info.rec_type.empty() && info.record_id.empty() && !is_merge)
			return node_kind_t::file_on_source;

		return node_kind_t::other;
	}();

	QMenu menu;

	switch (node_kind)
	{
	case node_kind_t::record_on_merge:
	{
		menu.addAction(
		    QCoreApplication::translate("yEditor", "Remove"),
		    [this, info]() { m_merge.remove_record_from_merge(info.rec_type, info.record_id); });
		break;
	}

	case node_kind_t::file_on_source:
	{
		const auto & filename = m_session.scan().plugin_filename(info.plugin_idx);
		const bool excluded = m_session.excluded_plugins().count(filename) > 0;
		const bool is_patch = m_session.patch_plugins().count(filename) > 0;

		menu.addAction(
		    excluded ? "Include in Merged Patch" : "Exclude from Merged Patch",
		    [this, filename, excluded]()
		{
			auto excluded_copy = m_session.excluded_plugins();
			if (excluded)
				excluded_copy.erase(filename);
			else
				excluded_copy.insert(filename);

			m_session.set_excluded_plugins(excluded_copy);
			m_session.save_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");
			m_nav_view.rebuild_preserving_state();
		});

		menu.addAction(
		    is_patch ? "Unmark as Guard Patch" : "Mark as Guard Patch",
		    [this, filename, is_patch]()
		{
			auto patch_copy = m_session.patch_plugins();
			if (is_patch)
				patch_copy.erase(filename);
			else
				patch_copy.insert(filename);

			m_session.set_patch_plugins(patch_copy);
			m_session.save_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");
			m_nav_view.rebuild_preserving_state();
		});
		break;
	}

	case node_kind_t::other:
		break;
	}

	if (menu.actions().isEmpty())
		return;

	menu.exec(global_pos);
}

void view_context_menu_t::show_view_menu(const QPoint & global_pos, const QModelIndex & index)
{
	if (!m_session.scan().has_merge())
		return;

	if (!index.isValid())
		return;

	const int col = index.column() - 1;
	if (col < 0 || col >= static_cast<int>(m_record_view.model()->column_plugin_indices().size()))
		return;

	const int plugin_idx = m_record_view.model()->column_plugin_indices()[col];
	const auto & rec_type = m_record_view.model()->record_type();
	const auto & record_id = m_record_view.model()->record_id();

	const auto * node = m_record_view.model()->node_from_index(index);
	if (!node)
		return;

	const auto & row = *node;
	const bool is_field_row = index.parent().isValid();
	const int parent_row_idx = is_field_row ? index.parent().row() : index.row();

	const auto & visible = m_record_view.model()->rows();
	if (parent_row_idx < 0 || parent_row_idx >= static_cast<int>(visible.size()))
		return;

	auto binary_start = [&](const view_tree_model_t::view_node_t & target) -> int
	{
		if (col < 0 || col >= static_cast<int>(target.binary_ranges.size()))
			return -1;

		return target.binary_ranges[col].start;
	};

	const int bin_idx = binary_start(row);

	enum class col_state_t
	{
		not_in_merge,
		source,
		merge
	};

	const bool is_on_merge = m_session.scan().is_merge_plugin(plugin_idx);
	const bool record_in_merge = m_session.scan().find_merge_content(rec_type, record_id) != nullptr;

	const auto col_state = [&]() -> col_state_t
	{
		if (is_on_merge)
			return col_state_t::merge;

		if (!record_in_merge)
			return col_state_t::not_in_merge;

		return col_state_t::source;
	}();

	enum class row_kind_t
	{
		sub_record,
		schema_record,
		group,
		field_of_schema,
		field_of_group,
		other
	};

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

	QMenu menu;

	switch (col_state)
	{
	case col_state_t::not_in_merge:
	{
		const auto * behavior = find_record_behavior(rec_type);

		if (behavior->copy_strategy == copy_strategy_t::header_and_selected_group)
		{
			menu.addAction(
			    "Copy Record to Merged Patch",
			    [this, plugin_idx, rec_type, record_id, index, col]()
			{ m_merge.copy_cell_record(plugin_idx, rec_type, record_id, index, col); });
		}
		else
		{
			menu.addAction(
			    "Copy Record to Merged Patch",
			    [this, plugin_idx, rec_type, record_id]()
			{ m_merge.copy_whole_record(plugin_idx, rec_type, record_id); });
		}

		break;
	}

	case col_state_t::source:
	{
		switch (kind)
		{
		case row_kind_t::sub_record:
		case row_kind_t::schema_record:
		{
			const auto sub_type = row.type;
			menu.addAction(
			    "Copy Sub-Record to Merged Patch",
			    [this, plugin_idx, rec_type, record_id, sub_type, bin_idx]()
			{ m_merge.copy_sub_record(plugin_idx, rec_type, record_id, sub_type, bin_idx); });
			break;
		}

		case row_kind_t::group:
		{
			menu.addAction(
			    "Copy Group to Merged Patch",
			    [this, plugin_idx, rec_type, record_id, parent_row_idx]()
			{ m_merge.copy_group(plugin_idx, rec_type, record_id, parent_row_idx); });
			break;
		}

		case row_kind_t::field_of_schema:
		{
			QModelIndex sub_record_index = index.parent();
			const auto * sub_record_node = m_record_view.model()->node_from_index(sub_record_index);

			while (sub_record_node && sub_record_node->type.empty() && sub_record_index.parent().isValid())
			{
				sub_record_index = sub_record_index.parent();
				sub_record_node = m_record_view.model()->node_from_index(sub_record_index);
			}

			if (!sub_record_node || sub_record_node->type.empty())
				break;

			if (row.schema_field_index < 0)
				break;

			const auto sub_type = sub_record_node->type;
			const auto sub_size = sub_record_node->size;
			const int field_bin = binary_start(*sub_record_node);
			const int child_field_idx = row.schema_field_index;
			menu.addAction(
			    "Copy Field to Merged Patch",
			    [this, plugin_idx, rec_type, record_id, sub_type, sub_size, field_bin, child_field_idx]()
			{ m_merge.copy_field(plugin_idx, rec_type, record_id, sub_type, sub_size, field_bin, child_field_idx); });
			break;
		}

		case row_kind_t::field_of_group:
		{
			menu.addAction(
			    "Copy Group to Merged Patch",
			    [this, plugin_idx, rec_type, record_id, parent_row_idx]()
			{ m_merge.copy_group(plugin_idx, rec_type, record_id, parent_row_idx); });
			break;
		}

		case row_kind_t::other:
			break;
		}

		break;
	}

	case col_state_t::merge:
	{
		switch (kind)
		{
		case row_kind_t::sub_record:
		case row_kind_t::schema_record:
		{
			if (bin_idx >= 0)
			{
				const auto removed_type = row.type;
				menu.addAction(
				    "Remove Sub-Record",
				    [this, rec_type, record_id, bin_idx, removed_type]()
				{ m_merge.remove_sub_record(rec_type, record_id, bin_idx, removed_type); });
			}

			break;
		}

		case row_kind_t::group:
		case row_kind_t::field_of_group:
		{
			auto merge_range = (kind == row_kind_t::field_of_group) ? visible[parent_row_idx].binary_ranges[col]
			                                                        : row.binary_ranges[col];

			if (merge_range.start >= 0)
			{
				menu.addAction(
				    "Remove Group",
				    [this, rec_type, record_id, merge_range]()
				{ m_merge.remove_group(rec_type, record_id, merge_range); });
			}

			break;
		}

		case row_kind_t::field_of_schema:
		{
			QModelIndex sub_record_index = index.parent();
			const auto * sub_record_node = m_record_view.model()->node_from_index(sub_record_index);

			while (sub_record_node && sub_record_node->type.empty() && sub_record_index.parent().isValid())
			{
				sub_record_index = sub_record_index.parent();
				sub_record_node = m_record_view.model()->node_from_index(sub_record_index);
			}

			if (sub_record_node && !sub_record_node->type.empty())
			{
				const int merge_bin = binary_start(*sub_record_node);
				if (merge_bin >= 0)
				{
					const auto removed_type = sub_record_node->type;
					menu.addAction(
					    "Remove Sub-Record",
					    [this, rec_type, record_id, merge_bin, removed_type]()
					{ m_merge.remove_sub_record(rec_type, record_id, merge_bin, removed_type); });
				}
			}

			break;
		}

		case row_kind_t::other:
			break;
		}

		break;
	}
	}

	if (menu.actions().isEmpty())
		return;

	menu.exec(global_pos);
}
