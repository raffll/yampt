#pragma once

#include "session.hpp"
#include <functional>
#include <string>

class log_view_t;
class QTabWidget;
class QWidget;

struct dict_operations_deps_t
{
	session_t & session;
	log_view_t & log_view;
	QTabWidget & record_tabs;
	QWidget * parent_widget;
	std::function<void()> scan_workspace;
	std::function<void(document_t *)> switch_document;
	std::function<void()> rebuild_sidebar;
};

class dict_operations_controller_t
{
public:
	explicit dict_operations_controller_t(dict_operations_deps_t deps);

	void on_merge();

private:
	dict_operations_deps_t m_deps;
};
