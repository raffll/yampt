#pragma once

#include "session.hpp"
#include "workspace_watcher.hpp"
#include <io/file_list.hpp>
#include <functional>
#include <string>
#include <unordered_map>

class log_view_t;
class sidebar_view_t;
class QWidget;

#include "../model/filter_state.hpp"

struct sidebar_controller_callbacks_t
{
	std::function<void(document_t *)> switch_document;
	std::function<void()> rebuild_annotations;
	std::function<void()> save_config;
	std::function<void(bool)> set_unsaved_changes;
};

struct sidebar_controller_deps_t
{
	session_t & session;
	file_list_t & file_list;
	workspace_watcher_t & workspace_watcher;
	sidebar_view_t & sidebar_view;
	log_view_t & log_view;
	std::unordered_map<std::string, filter_state_t> & filter_states;
	size_t & last_annotation_version;
	document_t *& active_doc;
	QWidget * parent_widget;
	sidebar_controller_callbacks_t callbacks;
};

class sidebar_controller_t
{
public:
	explicit sidebar_controller_t(sidebar_controller_deps_t deps);

	void on_item_clicked(const std::string & path);
	void on_save_requested(const std::string & path);
	void on_unload_requested(const std::string & path);
	void on_delete_requested(const std::string & path);
	void on_export_native_requested(const std::string & path);
	void scan_workspace();
	void update_watcher_roots();
	void rebuild_sidebar();
	void update_sidebar_item(const std::string & path);
	void on_remove_folder_requested(const std::string & root_path);
	void on_delete_folder_requested(const std::string & folder_path);

private:
	sidebar_controller_deps_t m_deps;
};
