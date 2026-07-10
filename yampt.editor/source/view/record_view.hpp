#pragma once

#include "../model/view_tree_model.hpp"
#include <QWidget>

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

	QTreeView * m_tree = nullptr;
	view_tree_model_t * m_model = nullptr;
};
