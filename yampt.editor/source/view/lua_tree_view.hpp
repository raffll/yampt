#pragma once

#include "../model/lua_tree_model.hpp"
#include <QWidget>

class QTreeView;

class lua_tree_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit lua_tree_view_t(QWidget * parent = nullptr);

	void set_scan_result(const lua_scan_result_t & result);
	void clear();

	lua_tree_model_t::node_info_t current_selection() const;
	const lua_scan_result_t & scan_result() const;

signals:
	void selection_changed(const lua_tree_model_t::node_info_t & info);

private:
	QTreeView * m_tree = nullptr;
	lua_tree_model_t * m_model = nullptr;
};
