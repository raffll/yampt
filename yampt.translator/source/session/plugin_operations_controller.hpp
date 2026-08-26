#pragma once

#include "../dialog/dict_selection_dialog.hpp"
#include "../editor/operation_executor.hpp"
#include "../model/make_base_params.hpp"
#include "../model/plugin_op.hpp"
#include "session.hpp"
#include <io/file_list.hpp>
#include <functional>
#include <optional>
#include <settings_store.hpp>
#include <string>
#include <vector>

class dict_document_t;
class log_view_t;
class translation_suggestion_view_t;
class QTabWidget;
class QString;
class QWidget;

struct plugin_operations_callbacks_t
{
	std::function<void()> scan_workspace;
	std::function<std::optional<make_base_params_t>(const std::string &)> show_make_base_dialog;
};

struct plugin_operations_deps_t
{
	session_t & session;
	file_list_t & file_list;
	settings_store_t & settings;
	operation_executor_t & executor;
	log_view_t & log_view;
	translation_suggestion_view_t & translation_suggestion_view;
	QTabWidget & record_tabs;
	QWidget * parent_widget;
	plugin_operations_callbacks_t callbacks;
};

class plugin_operations_controller_t
{
public:
	explicit plugin_operations_controller_t(plugin_operations_deps_t deps);

	void on_plugin_operation(const std::string & plugin_path, plugin_op_t operation);
	void log_operation_result(
	    const std::string & plugin_path,
	    plugin_op_t operation_type,
	    const operation_executor_t::result_t & result);
	std::vector<dict_selection_dialog_t::dict_entry_t> build_dict_entries(const std::string & source_dir = {}) const;

private:
	operation_executor_t::result_t execute_make_dict(const std::string & plugin_path, const std::string & plugin_dir);
	operation_executor_t::result_t execute_make_base(const std::string & plugin_path);
	operation_executor_t::result_t execute_convert(const std::string & plugin_path, const std::string & plugin_dir);
	operation_executor_t::result_t execute_convert_hyperlinks(const std::string & plugin_path, const std::string & plugin_dir);
	operation_executor_t::result_t execute_create_plugin(const std::string & plugin_path, const std::string & plugin_dir);
	std::optional<std::vector<std::string>> show_dict_selection(const std::string & plugin_dir, const QString & title);

	plugin_operations_deps_t m_deps;
};
