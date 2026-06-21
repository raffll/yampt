#pragma once

#include "nav_tree_model.hpp"
#include "view_tree_model.hpp"
#include "messages_panel.hpp"
#include "../yampt/plugin_scan/plugin_scan.hpp"
#include <QWidget>
#include <QTreeView>
#include <QSplitter>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QMenu>

class editor_tab_t : public QWidget
{
	Q_OBJECT

public:
	explicit editor_tab_t(QWidget * parent = nullptr);

public slots:
	void on_load_plugins();
	void on_load_data_files();
	void on_load_mo2_profile();
	void on_load_openmw_cfg();
	void on_new_plugin();
	void on_save_plugin();

private slots:
	void on_create_merged_patch();
	void on_nav_selection_changed(const QModelIndex & current);
	void on_filter_changed();
	void on_advanced_filter();
	void on_remove_itm();
	void on_nav_context_menu(const QPoint & pos);
	void on_view_copy();

private:
	void rebuild_after_load();
	void update_status();
	void log_message(const std::string & msg);
	void save_plugin_paths();
	void load_plugin_paths();
	void rebuild_nav_preserving_state();
	void load_plugins_from_paths(const std::vector<std::string> & paths);
	bool eventFilter(QObject * obj, QEvent * event) override;

	plugin_scan_t scan_;

	QPushButton * btn_load_ = nullptr;
	QPushButton * btn_new_ = nullptr;
	QPushButton * btn_save_ = nullptr;
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
	messages_panel_t * messages_ = nullptr;

	QLabel * status_label_ = nullptr;

	bool filter_active_ = false;
	nav_tree_model_t::filter_state_t last_filter_state_;
};
