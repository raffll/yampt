#pragma once

#include "../editor/glossary.hpp"
#include "../model/dict_document.hpp"
#include "session.hpp"
#include <functional>
#include <string>

class log_view_t;
class translation_suggestion_view_t;
class QTabWidget;
class QWidget;

struct dict_operations_deps_t
{
	session_t & session;
	glossary_t & glossary;
	log_view_t & log_view;
	translation_suggestion_view_t & translation_view;
	QTabWidget & record_tabs;
	QWidget * parent_widget;
	std::function<void()> scan_workspace;
	std::function<void(document_t *)> switch_document;
	std::function<void()> rebuild_sidebar;
	std::function<void(const std::string &)> update_sidebar_item;
	std::function<void(bool)> set_unsaved_changes;
	std::function<void(int)> load_record;
	std::function<int()> current_row;
};

class dict_operations_controller_t
{
public:
	explicit dict_operations_controller_t(dict_operations_deps_t deps);

	void on_merge();
	void start_batch_translation(dict_document_t * dict_doc);

private:
	dict_operations_deps_t m_deps;
};
