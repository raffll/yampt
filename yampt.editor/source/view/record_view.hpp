#pragma once

#include "../model/view_tree_model.hpp"
#include <QWidget>
#include <set>
#include <string>

class QTreeView;
class plugin_scan_t;

class record_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit record_view_t(QWidget * parent = nullptr);

	void display_record(plugin_scan_t & scan, const conflict_entry_t & entry);
	void clear();
	void resize_columns();
	void refresh_expansion();

	view_tree_model_t * model() const;
	QTreeView * tree() const;

signals:
	void context_menu_requested(const QPoint & global_pos, const QModelIndex & index);
	void selection_changed(const QModelIndex & current);

private:
	void setup_tree();
	void expand_non_numeric_groups();
	void apply_column_sizing();
	void resizeEvent(QResizeEvent * event) override;

	static std::string node_expansion_key(const view_tree_model_t::view_node_t & node);
	std::set<std::string> capture_expanded_keys() const;
	void restore_expanded_keys(const std::set<std::string> & expanded_keys);

	QTreeView * m_tree = nullptr;
	view_tree_model_t * m_model = nullptr;
	std::string m_displayed_record_type;
	std::string m_displayed_record_id;
};
