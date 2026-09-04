#pragma once

#include "session.hpp"
#include <functional>
#include <string>

class log_view_t;
class edit_history_t;
class dict_document_t;
class QWidget;

struct dict_operations_deps_t
{
	session_t & session;
	log_view_t & log_view;
	QWidget * parent_widget;
	edit_history_t & edit_history;
	std::function<void()> scan_workspace;
	std::function<void(document_t *)> switch_document;
	std::function<void()> rebuild_sidebar;
	std::function<dict_document_t *()> active_dict_document;
	std::function<void()> refresh_table;
	std::function<void()> rebuild_annotations;
	std::function<void()> update_annotations;
};

class dict_operations_controller_t
{
public:
	explicit dict_operations_controller_t(dict_operations_deps_t deps);

	void on_merge();
	void on_apply_tags(dict_document_t * dict_doc);
	void on_remove_tags(dict_document_t * dict_doc);

private:
	void refresh_after_tagging();

	dict_operations_deps_t m_deps;
};
