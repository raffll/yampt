#pragma once

#include "../model/nav_tree_model.hpp"
#include "../model/view_tree_model.hpp"
#include "messages_view.hpp"
#include <plugin_scan/plugin_scan.hpp>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QTreeView>
#include <QWidget>

class QDropEvent;

class plugin_workspace_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit plugin_workspace_view_t(QWidget * parent = nullptr);

public slots:
	void on_load_plugins();
	void on_load_data_files();
	void on_load_mo2_profile();
	void on_load_openmw_cfg();
	void on_new_plugin();
	void on_save_plugin();
	void on_unload_all();

private slots:
	void on_create_merged_patch();
	void on_nav_selection_changed(const QModelIndex & current);
	void on_filter_changed();
	void on_advanced_filter();
	void on_remove_itm();
	void on_nav_context_menu(const QPoint & pos);
	void on_view_copy();

private:
	void setup_toolbar();
	void setup_views();
	void setup_connections();
	void rebuild_after_load();
	void update_status();
	void log_message(const std::string & msg);
	void save_plugin_paths();
	void load_plugin_paths();
	void rebuild_nav_preserving_state();
	void load_plugins_from_paths(const std::vector<std::string> & paths);
	void display_record_in_view(const conflict_entry_t & entry);
	std::vector<std::string> parse_mo2_profile(const QString & profile_dir);
	std::vector<std::string> parse_openmw_cfg(const QString & cfg_path);

	struct mo2_resolve_context_t
	{
		std::vector<std::string> enabled_mods;
		QString mods_path;
		QString game_data_path;
	};

	std::vector<std::string> resolve_mo2_plugins(
	    const std::vector<std::string> & plugin_names,
	    const mo2_resolve_context_t & context);
	std::string resolve_single_mo2_plugin(const std::string & plugin_name, const mo2_resolve_context_t & context);
	std::vector<std::string> resolve_openmw_content(
	    const std::vector<std::string> & content_names,
	    const std::vector<std::string> & data_dirs);
	std::string resolve_single_content(const std::string & content_name, const std::vector<std::string> & data_dirs);
	int create_merge_records();
	bool eventFilter(QObject * obj, QEvent * event) override;
	bool handle_drop_on_view(QDropEvent * drop_event);
	bool handle_drop_on_nav(QDropEvent * drop_event);
	void refresh_after_merge(const std::string & rec_type, const std::string & record_id);

	plugin_scan_t scan_;

	QPushButton * btn_load_ = nullptr;
	QPushButton * btn_new_ = nullptr;
	QPushButton * btn_save_ = nullptr;
	QPushButton * btn_merge_ = nullptr;
	QPushButton * btn_filter_ = nullptr;
	QCheckBox * chk_conflicts_ = nullptr;
	QComboBox * cmb_type_filter_ = nullptr;
	QLineEdit * edt_search_ = nullptr;
	QLabel * lbl_count_ = nullptr;

	QSplitter * main_splitter_ = nullptr;
	QSplitter * content_splitter_ = nullptr;
	QTreeView * nav_view_ = nullptr;
	QTreeView * view_view_ = nullptr;
	nav_tree_model_t * nav_model_ = nullptr;
	view_tree_model_t * view_model_ = nullptr;
	messages_view_t * messages_ = nullptr;

	QLabel * status_label_ = nullptr;

	bool filter_active_ = false;
	nav_tree_model_t::filter_state_t last_filter_state_;

	bool has_filter_active_ = false;
	nav_tree_model_t::filter_state_t last_quick_filter_;
};
