#pragma once

#include "../model/nav_tree_model.hpp"
#include <set>
#include <string>
#include <QWidget>

class QTreeView;
class plugin_scan_t;

class nav_tree_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit nav_tree_view_t(plugin_scan_t & scan, QWidget * parent = nullptr);

	void rebuild();
	void rebuild_preserving_state();
	void set_filter(const nav_tree_model_t::filter_state_t & state);
	void clear_filter();
	void set_hide_duplicates(bool hide);
	void set_show_deleted_strikeout(bool value);
	void set_excluded_plugins(const std::set<std::string> * excluded);
	void set_patch_plugins(const std::set<std::string> * patch);

	nav_tree_model_t::node_info_t current_selection() const;
	nav_tree_model_t::node_info_t node_at(const QModelIndex & index) const;
	QModelIndex find_index(const std::string & rec_type, const std::string & record_id) const;
	QModelIndex parent_index(const QModelIndex & index) const;

	QTreeView * tree_widget() const;

signals:
	void selection_changed(const nav_tree_model_t::node_info_t & info);
	void context_menu_requested(const QPoint & global_pos, const nav_tree_model_t::node_info_t & info);

private:
	bool eventFilter(QObject * obj, QEvent * event) override;
	void save_expansion_state();
	void restore_expansion_state();

	QTreeView * m_tree = nullptr;
	nav_tree_model_t * m_model = nullptr;
	std::set<std::string> m_expanded_items;
};
