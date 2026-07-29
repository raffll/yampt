#pragma once

#include <scanner/lua_scanner.hpp>
#include <QWidget>

class QLabel;
class QSplitter;
class QTreeView;
class lua_detail_view_t;
class lua_filter_view_t;
class lua_nav_model_t;
class lua_registrations_model_t;

class lua_conflicts_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit lua_conflicts_view_t(QWidget * parent = nullptr);

	void set_scan_result(const lua_scan_result_t & result);

private:
	void setup_layout();
	void apply_filter();
	void on_conflict_selection_changed();
	void on_registration_selection_changed();
	void show_conflicts_mode(const lua_scan_result_t & result);
	void show_registrations_mode(const lua_scan_result_t & result);

	QSplitter * m_splitter = nullptr;
	QTreeView * m_tree_view = nullptr;
	QLabel * m_empty_label = nullptr;
	QWidget * m_left_widget = nullptr;
	lua_nav_model_t * m_nav_model = nullptr;
	lua_registrations_model_t * m_registrations_model = nullptr;
	lua_detail_view_t * m_detail_view = nullptr;
	lua_filter_view_t * m_filter_view = nullptr;
	bool m_in_conflicts_mode = false;
};
