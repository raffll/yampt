#pragma once

#include "../model/nav_tree_model.hpp"
#include "../model/view_tree_model.hpp"
#include "messages_view.hpp"
#include <scanner/plugin_scan.hpp>
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
class app_settings_t;

class plugin_workspace_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit plugin_workspace_view_t(app_settings_t & settings, QWidget * parent = nullptr);

	void save_session_state();
	void restore_session_state();

public slots:
	void on_load_plugins();
	void on_load_data_files();
	void on_load_mo2_profile();
	void on_load_openmw_cfg();
	void on_new_plugin();
	void on_save_plugin();
	void on_unload_all();
	void on_create_merged_patch();
	void on_advanced_filter();

private slots:
	void on_nav_selection_changed(const QModelIndex & current);
	void on_filter_changed();
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

	app_settings_t & m_settings;
	plugin_scan_t m_scan;

	QPushButton * m_btn_load = nullptr;
	QPushButton * m_btn_new = nullptr;
	QPushButton * m_btn_save = nullptr;
	QPushButton * m_btn_merge = nullptr;
	QPushButton * m_btn_filter = nullptr;
	QCheckBox * m_chk_conflicts = nullptr;
	QComboBox * m_cmb_type_filter = nullptr;
	QLineEdit * m_edt_search = nullptr;
	QLabel * m_lbl_count = nullptr;

	QSplitter * m_main_splitter = nullptr;
	QSplitter * m_content_splitter = nullptr;
	QTreeView * m_nav_view = nullptr;
	QTreeView * m_view_view = nullptr;
	nav_tree_model_t * m_nav_model = nullptr;
	view_tree_model_t * m_view_model = nullptr;
	messages_view_t * m_messages = nullptr;

	QLabel * m_status_label = nullptr;

	bool m_filter_active = false;
	nav_tree_model_t::filter_state_t m_last_filter_state;

	bool m_has_filter_active = false;
	nav_tree_model_t::filter_state_t m_last_quick_filter;
};
