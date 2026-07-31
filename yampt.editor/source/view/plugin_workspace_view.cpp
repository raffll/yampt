#include "plugin_workspace_view.hpp"
#include "count_label_format.hpp"
#include "../dialog/filter_dialog.hpp"
#include "../dialog/plugin_select_dialog.hpp"
#include "../session/lua_scan_worker.hpp"
#include "editor_delegates.hpp"

#include <scanner/batch_cleaner.hpp>
#include <scanner/record_conflict.hpp>
#include <set>
#include <settings_store.hpp>
#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSettings>
#include <QShortcut>
#include <QTreeView>
#include <QVBoxLayout>

plugin_workspace_view_t::plugin_workspace_view_t(settings_store_t & settings, QWidget * parent)
    : QWidget(parent)
    , m_settings(settings)
{
	auto * main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(4, 4, 4, 4);

	m_lbl_count = new QLabel(this);

	m_session = new plugin_session_t(this);
	apply_user_conflict_rules();

	setup_views();

	m_edit_controller = new field_edit_controller_t(*m_session, this);
	m_preview->set_edit_controller(m_edit_controller);
	m_preview->set_editable_columns(&m_editable_columns);

	main_layout->addWidget(m_main_splitter, 1);

	m_status_label = new QLabel(this);

	m_nav_view = new nav_tree_view_t(m_session->scan(), this);
	m_nav_view->set_excluded_plugins(&m_session->excluded_plugins());
	m_nav_view->set_patch_plugins(&m_session->patch_plugins());
	m_nav_view->set_editable_columns(&m_editable_columns);
	m_content_splitter->insertWidget(0, m_nav_view);
	m_record_view = new record_view_t(this);
	m_record_view->model()->set_excluded_plugins(&m_session->excluded_plugins());
	m_record_view->model()->set_patch_plugins(&m_session->patch_plugins());
	m_record_view->model()->set_display_codepage(static_cast<codepage_t>(m_settings.display_codepage()));
	m_record_view->model()->set_user_ignore_conflict(m_session->scan().user_ignore_conflict());
	m_nav_view->set_display_codepage(static_cast<codepage_t>(m_settings.display_codepage()));
	m_content_splitter->insertWidget(1, m_record_view);
	m_content_splitter->setSizes({ 200, 800 });

	m_merge_controller = new merge_controller_t(
	    *m_session, *m_record_view, *m_nav_view, m_settings, [this](const std::string & msg) { log_message(msg); });

	m_context_menu = new view_context_menu_t(*m_session, *m_record_view, *m_nav_view, *m_merge_controller, m_editable_columns);

	setup_connections();
}

void plugin_workspace_view_t::setup_views()
{
	m_main_splitter = new QSplitter(Qt::Vertical, this);
	m_content_splitter = new QSplitter(Qt::Horizontal);

	m_lua_scan_worker = new lua_scan_worker_t(this);

	m_bottom_tabs = new QTabWidget(m_main_splitter);
	m_messages = new messages_view_t(m_bottom_tabs);
	m_preview = new preview_view_t(m_bottom_tabs);
	m_bottom_tabs->addTab(m_messages, tr("Log"));
	m_bottom_tabs->addTab(m_preview, tr("Preview"));

	m_main_splitter->addWidget(m_content_splitter);
	m_main_splitter->addWidget(m_bottom_tabs);
	m_main_splitter->setSizes({ 600, 150 });
	m_main_splitter->setChildrenCollapsible(true);
}

void plugin_workspace_view_t::setup_connections()
{
	connect(m_nav_view, &nav_tree_view_t::selection_changed, this, &plugin_workspace_view_t::on_nav_selection_changed);
	connect(m_nav_view, &nav_tree_view_t::context_menu_requested, this, &plugin_workspace_view_t::on_nav_context_menu);

	connect(
	    m_record_view, &record_view_t::context_menu_requested, this, &plugin_workspace_view_t::on_view_context_menu);
	connect(
	    m_record_view, &record_view_t::selection_changed, this, &plugin_workspace_view_t::on_view_selection_changed);

	connect(m_session, &plugin_session_t::plugins_loaded, this, &plugin_workspace_view_t::rebuild_after_load);
	connect(
	    m_session,
	    &plugin_session_t::plugins_unloaded,
	    this,
	    [this]()
	{
		if (m_lua_scan_worker->isRunning())
			m_lua_scan_worker->cancel_scan();

		m_nav_view->clear_lua_section();
		m_record_view->clear();
		m_nav_view->rebuild();
		update_status();
	});
	connect(m_session, &plugin_session_t::log_message, this, &plugin_workspace_view_t::log_message);

	connect(m_lua_scan_worker, &lua_scan_worker_t::scan_complete, this, &plugin_workspace_view_t::on_lua_scan_complete);

	auto * copy_shortcut = new QShortcut(QKeySequence::Copy, m_record_view->tree());
	connect(copy_shortcut, &QShortcut::activated, this, &plugin_workspace_view_t::on_view_copy);

	connect(m_edit_controller, &field_edit_controller_t::record_modified, this, [this]() {
		rebuild_nav_preserving_state();
		m_merge_controller->save_merged_patch();
	});

	connect(m_preview, &preview_view_t::edit_committed, this, [this]() {
		const auto * model = m_record_view->model();
		const auto & rec_type = model->record_type();
		const auto & record_id = model->record_id();

		const auto * entry = m_session->scan().find(rec_type, record_id);
		if (!entry)
			return;

		display_record_in_view(*entry);
	});
}

void plugin_workspace_view_t::load_plugins_from_paths(
    const std::vector<std::string> & paths,
    const std::string & base_path)
{
	plugin_select_dialog_t dlg(paths, this);
	if (dlg.exec() != QDialog::Accepted)
		return;

	const auto selected = dlg.selected_paths();
	if (selected.empty())
		return;

	m_session->load_from_folder(selected, base_path);
}

void plugin_workspace_view_t::on_load_data_files()
{
	const auto initial_dir = QString::fromStdString(m_settings.last_directory());
	QString dir = QFileDialog::getExistingDirectory(this, tr("Select Data Files Folder"), initial_dir);

	if (dir.isEmpty())
		return;

	QDir data_dir(dir);
	auto file_list = data_dir.entryInfoList({ "*.esm", "*.esp" }, QDir::Files, QDir::Time | QDir::Reversed);

	std::vector<std::string> esms;
	std::vector<std::string> esps;

	for (const auto & fi : file_list)
	{
		auto path = fi.absoluteFilePath().toStdString();
		if (fi.suffix().toLower() == "esm")
			esms.push_back(path);
		else
			esps.push_back(path);
	}

	std::vector<std::string> paths;
	paths.insert(paths.end(), esms.begin(), esms.end());
	paths.insert(paths.end(), esps.begin(), esps.end());

	if (paths.empty())
	{
		log_message("No ESM/ESP files found in " + dir.toStdString());
		return;
	}

	load_plugins_from_paths(paths, dir.toStdString());
	m_settings.set_last_directory(dir.toStdString());
	m_merge_controller->load_existing_merged_patch();
}

void plugin_workspace_view_t::on_load_mo2_profile()
{
	const auto initial_dir = QString::fromStdString(m_settings.last_directory());
	QString profile_dir = QFileDialog::getExistingDirectory(this, tr("Select MO2 Profile Folder"), initial_dir);

	if (profile_dir.isEmpty())
		return;

	m_session->load_from_mo2_profile(profile_dir);
	m_settings.set_last_directory(profile_dir.toStdString());
}

void plugin_workspace_view_t::on_load_openmw_cfg()
{
	const auto initial_dir = QString::fromStdString(m_settings.last_directory());

	QString cfg_path =
	    QFileDialog::getOpenFileName(this, tr("Select openmw.cfg"), initial_dir, "OpenMW config (openmw.cfg)");

	if (cfg_path.isEmpty())
		return;

	m_session->load_from_openmw_cfg(cfg_path);
	const auto cfg_dir = QFileInfo(cfg_path).absolutePath();
	m_settings.set_last_directory(cfg_dir.toStdString());
}

void plugin_workspace_view_t::on_unload_all()
{
	m_session->unload_all();
}

void plugin_workspace_view_t::on_create_merged_patch()
{
	m_merge_controller->create_merged_patch();

	const auto info = m_nav_view->current_selection();
	if (info.record_id.empty())
	{
		update_status();
		return;
	}

	const auto * updated = m_session->scan().find(info.rec_type, info.record_id);
	if (!updated)
	{
		update_status();
		return;
	}

	display_record_in_view(*updated);
	update_status();
}

void plugin_workspace_view_t::on_clean_all()
{
	if (m_session->scan().plugin_count() < 2)
	{
		log_message("[error] need at least 2 plugins loaded to clean");
		return;
	}

	const auto output_path = m_merge_controller->resolve_output_directory();
	if (output_path.empty())
	{
		log_message("[error] cannot determine output directory");
		return;
	}

	log_message("[info] clean: output=" + output_path);

	batch_cleaner_t cleaner(m_session->scan(), [this](const std::string & message) { log_message(message); });

	clean_options_t options;
	options.evil_gmst = m_settings.clean_evil_gmst_enabled();
	options.junk_cell = m_settings.clean_junk_cell_enabled();
	options.update_master_sizes = m_settings.clean_update_master_sizes();
	options.update_version = m_settings.clean_update_version();
	cleaner.set_options(options);

	const auto results = cleaner.clean_all(output_path);
	if (results.empty())
		log_message("[info] no records to clean");
}

void plugin_workspace_view_t::rebuild_after_load()
{
	rebuild_nav_preserving_state();
	on_filter_changed();
	update_status();
	start_lua_scan();
}

void plugin_workspace_view_t::apply_user_conflict_rules()
{
	const auto rules_str = m_settings.sub_record_ignore_conflict();
	std::set<std::string> rules;
	size_t start = 0;

	while (start < rules_str.size())
	{
		const auto comma = rules_str.find(',', start);
		const auto end = (comma == std::string::npos) ? rules_str.size() : comma;

		auto token_start = start;
		while (token_start < end && rules_str[token_start] == ' ')
			++token_start;

		auto token_end = end;
		while (token_end > token_start && rules_str[token_end - 1] == ' ')
			--token_end;

		if (token_end > token_start)
			rules.insert(rules_str.substr(token_start, token_end - token_start));

		start = (comma == std::string::npos) ? rules_str.size() : comma + 1;
	}

	m_session->scan().set_user_ignore_conflict(rules);
}

void plugin_workspace_view_t::on_settings_changed()
{
	apply_user_conflict_rules();
	m_record_view->model()->set_user_ignore_conflict(m_session->scan().user_ignore_conflict());

	if (m_session->scan().plugin_count() > 0)
	{
		m_session->scan().rebuild_conflicts();
		rebuild_nav_preserving_state();

		const auto selection = m_nav_view->current_selection();
		if (!selection.rec_type.empty())
			on_nav_selection_changed(selection);
	}

	m_record_view->model()->set_display_codepage(static_cast<codepage_t>(m_settings.display_codepage()));
	m_nav_view->set_display_codepage(static_cast<codepage_t>(m_settings.display_codepage()));
}

void plugin_workspace_view_t::rebuild_nav_preserving_state()
{
	m_nav_view->rebuild_preserving_state();
}

void plugin_workspace_view_t::on_nav_selection_changed(const nav_tree_model_t::node_info_t & info)
{
	if (info.is_lua_node)
	{
		dispatch_lua_selection(info);
		update_status();
		return;
	}

	if (info.rec_type.empty())
	{
		m_record_view->clear();
		update_status();
		return;
	}

	const auto * entry = m_session->scan().find(info.rec_type, info.record_id);
	if (!entry)
	{
		m_record_view->clear();
		update_status();
		return;
	}

	display_record_in_view(*entry);
	update_status();
}

void plugin_workspace_view_t::dispatch_lua_selection(const nav_tree_model_t::node_info_t & info)
{
	if (info.lua_conflict_idx >= 0)
	{
		const auto idx = static_cast<size_t>(info.lua_conflict_idx);
		if (idx < m_lua_scan_result.conflicts.size())
			m_record_view->model()->set_lua_conflict(m_lua_scan_result.conflicts[idx]);
		return;
	}

	if (info.lua_registration_idx >= 0)
	{
		const auto idx = static_cast<size_t>(info.lua_registration_idx);
		if (idx < m_lua_scan_result.registrations.size())
			m_record_view->model()->set_lua_registration(m_lua_scan_result.registrations[idx]);
		return;
	}

	m_record_view->clear();
}

void plugin_workspace_view_t::set_conflicts_only(bool value)
{
	m_conflicts_only = value;
	on_filter_changed();
}

void plugin_workspace_view_t::set_show_deleted_strikeout(bool value)
{
	m_record_view->model()->set_show_deleted_strikeout(value);
	m_nav_view->set_show_deleted_strikeout(value);
}

bool plugin_workspace_view_t::is_show_deleted_strikeout() const
{
	return m_record_view->model()->show_deleted_strikeout();
}

void plugin_workspace_view_t::on_filter_changed()
{
	nav_tree_model_t::filter_state_t state;

	if (m_conflicts_only)
	{
		state.filter_conflict_all = true;
		state.conflict_all_set.insert(conflict_all_t::conflict);
		state.conflict_all_set.insert(conflict_all_t::override_benign);
	}

	bool has_any = state.filter_conflict_all;

	if (has_any)
	{
		if (m_has_filter_active && state == m_last_quick_filter)
			return;

		m_has_filter_active = true;
		m_last_quick_filter = state;
		m_nav_view->set_filter(state);
	}
	else
	{
		if (!m_has_filter_active)
			return;

		m_has_filter_active = false;
		m_last_quick_filter = {};
		m_nav_view->clear_filter();
	}

	update_status();
}

void plugin_workspace_view_t::set_hide_duplicates(bool hide)
{
	m_hide_duplicates = hide;
	m_nav_view->set_hide_duplicates(hide);

	const auto info = m_nav_view->current_selection();
	if (info.record_id.empty())
		return;

	const auto * entry = m_session->scan().find(info.rec_type, info.record_id);
	if (entry)
		display_record_in_view(*entry);
}

void plugin_workspace_view_t::on_advanced_filter()
{
	auto types = m_session->scan().all_types();
	filter_dialog_t dlg(types, this);

	if (m_filter_active)
	{
		filter_dialog_t::filter_state_t dlg_state;
		dlg_state.filter_conflict_all = m_last_filter_state.filter_conflict_all;
		dlg_state.conflict_all_set = m_last_filter_state.conflict_all_set;
		dlg_state.filter_conflict_this = m_last_filter_state.filter_conflict_this;
		dlg_state.conflict_this_set = m_last_filter_state.conflict_this_set;
		dlg_state.filter_by_type = m_last_filter_state.filter_by_type;
		dlg_state.type_set = m_last_filter_state.type_set;
		dlg_state.filter_by_id = m_last_filter_state.filter_by_id;
		dlg_state.id_text = m_last_filter_state.id_text;
		dlg_state.filter_by_name = m_last_filter_state.filter_by_name;
		dlg_state.name_text = m_last_filter_state.name_text;
		dlg_state.filter_deleted = m_last_filter_state.filter_deleted;
		dlg.set_state(dlg_state);
	}

	if (dlg.exec() != QDialog::Accepted)
		return;

	auto state = dlg.state();

	nav_tree_model_t::filter_state_t nav_state;
	nav_state.filter_conflict_all = state.filter_conflict_all;
	nav_state.conflict_all_set = state.conflict_all_set;
	nav_state.filter_conflict_this = state.filter_conflict_this;
	nav_state.conflict_this_set = state.conflict_this_set;
	nav_state.filter_by_type = state.filter_by_type;
	nav_state.type_set = state.type_set;
	nav_state.filter_by_id = state.filter_by_id;
	nav_state.id_text = state.id_text;
	nav_state.filter_by_name = state.filter_by_name;
	nav_state.name_text = state.name_text;
	nav_state.filter_deleted = state.filter_deleted;

	m_last_filter_state = nav_state;
	m_filter_active = true;
	m_has_filter_active = true;
	m_last_quick_filter = nav_state;

	m_nav_view->set_filter(nav_state);
	update_status();
}

void plugin_workspace_view_t::on_nav_context_menu(const QPoint & global_pos, const nav_tree_model_t::node_info_t & info)
{
	m_context_menu->show_nav_menu(global_pos, info);
}

void plugin_workspace_view_t::on_view_context_menu(const QPoint & global_pos, const QModelIndex & index)
{
	m_context_menu->show_view_menu(global_pos, index);
}

void plugin_workspace_view_t::on_view_copy()
{
	auto indexes = m_record_view->tree()->selectionModel()->selectedIndexes();
	if (indexes.isEmpty())
		return;

	QStringList texts;
	for (const auto & idx : indexes)
	{
		auto text = idx.data(Qt::DisplayRole).toString();
		if (!text.isEmpty())
			texts.append(text);
	}

	if (!texts.isEmpty())
		QApplication::clipboard()->setText(texts.join("\t"));
}

void plugin_workspace_view_t::on_view_selection_changed(const QModelIndex & current)
{
	if (!current.isValid() || !m_preview)
	{
		m_preview->clear();
		return;
	}

	const auto * model = m_record_view->model();
	const int clicked_col = current.column();
	if (clicked_col < 1)
	{
		m_preview->clear();
		return;
	}

	m_preview->set_editing_enabled(false);

	const auto right_variant = model->data(current, Qt::DisplayRole);
	const auto right_text = right_variant.isValid() ? right_variant.toString().toStdString() : std::string {};

	const auto left_index = model->index(current.row(), clicked_col - 1, current.parent());
	std::string left_text;
	if (left_index.isValid() && left_index.column() >= 1)
	{
		const auto left_variant = model->data(left_index, Qt::DisplayRole);
		left_text = left_variant.isValid() ? left_variant.toString().toStdString() : std::string {};
	}

	if (right_text.empty() && left_text.empty())
		m_preview->clear();
	else
		m_preview->show_comparison(left_text, right_text);

	m_preview->update_selection(current, model, right_text);
}

void plugin_workspace_view_t::display_record_in_view(const conflict_entry_t & entry)
{
	if (m_hide_duplicates && entry.versions.size() > 1)
	{
		conflict_entry_t filtered;
		filtered.rec_type = entry.rec_type;
		filtered.record_id = entry.record_id;
		filtered.display_name = entry.display_name;
		filtered.dial_name = entry.dial_name;
		filtered.conflict_all = entry.conflict_all;

		std::set<int> seen_plugins;
		for (auto it = entry.versions.rbegin(); it != entry.versions.rend(); ++it)
		{
			if (seen_plugins.count(it->plugin_idx))
				continue;

			seen_plugins.insert(it->plugin_idx);
			filtered.versions.insert(filtered.versions.begin(), *it);
		}

		m_record_view->display_record(m_session->scan(), filtered);
	}
	else
	{
		m_record_view->display_record(m_session->scan(), entry);
	}

	const auto * view_model = m_record_view->model();
	m_editable_columns.rebuild_for_record(view_model->column_plugin_indices(), view_model->merge_column());
}

void plugin_workspace_view_t::update_status()
{
	size_t plugin_count = m_session->scan().plugin_count();
	size_t record_count = m_session->scan().entries().size();
	size_t conflict_count = 0;

	for (const auto & entry : m_session->scan().entries())
	{
		if (entry.conflict_all == conflict_all_t::conflict)
			++conflict_count;
	}

	const auto lua_conflict_count = m_lua_scan_result.conflicts.size();
	m_lbl_count->setText(count_label_format::format(plugin_count, record_count, conflict_count, lua_conflict_count));

	const auto info = m_nav_view->current_selection();

	auto mode_prefix = build_mode_prefix();

	if (info.plugin_idx >= 0)
	{
		if (!info.record_id.empty())
			m_status_label->setText(mode_prefix + QString::fromStdString(info.rec_type + " : " + info.record_id));
		else if (!info.rec_type.empty())
			m_status_label->setText(mode_prefix + QString::fromStdString(info.rec_type));
		else
			m_status_label->setText(
			    mode_prefix + QString::fromStdString(m_session->scan().plugin_filename(info.plugin_idx)));
	}
	else
	{
		m_status_label->setText(mode_prefix.trimmed());
	}
}

QString plugin_workspace_view_t::build_mode_prefix() const
{
	if (!m_session || m_session->load_base_path().empty())
		return {};

	QString mode_tag;
	switch (m_session->load_source())
	{
	case plugin_session_t::load_source_t::folder:
		mode_tag = "[Folder]";
		break;
	case plugin_session_t::load_source_t::mo2_profile:
		mode_tag = "[MO2]";
		break;
	case plugin_session_t::load_source_t::openmw_cfg:
		mode_tag = "[OpenMW]";
		break;
	default:
		return {};
	}

	return mode_tag + " " + QString::fromStdString(m_session->load_base_path()) + " : ";
}

void plugin_workspace_view_t::log_message(const std::string & msg)
{
	m_messages->log(msg);
}

void plugin_workspace_view_t::save_session_state()
{
	const auto ini_path = QCoreApplication::applicationDirPath() + "/yEditor.ini";

	m_session->save_session_state(ini_path);

	QSettings settings(ini_path, QSettings::IniFormat);

	settings.setValue("session/main_splitter", m_main_splitter->saveState());
	settings.setValue("session/content_splitter", m_content_splitter->saveState());
	settings.setValue("view/conflicts_only", m_conflicts_only);
	settings.setValue("view/hide_duplicates", m_hide_duplicates);
	settings.setValue("view/show_deleted_strikeout", m_record_view->model()->show_deleted_strikeout());
	settings.setValue("view/nav_header", m_nav_view->tree_widget()->header()->saveState());

	const auto info = m_nav_view->current_selection();
	if (info.plugin_idx >= 0 && !info.record_id.empty())
	{
		settings.setValue("session/nav_rec_type", QString::fromStdString(info.rec_type));
		settings.setValue("session/nav_record_id", QString::fromStdString(info.record_id));
	}
	else
	{
		settings.remove("session/nav_rec_type");
		settings.remove("session/nav_record_id");
	}
}

void plugin_workspace_view_t::restore_session_state()
{
	QSettings settings(QCoreApplication::applicationDirPath() + "/yEditor.ini", QSettings::IniFormat);

	m_conflicts_only = settings.value("view/conflicts_only", false).toBool();
	m_hide_duplicates = settings.value("view/hide_duplicates", false).toBool();

	m_record_view->model()->set_show_deleted_strikeout(settings.value("view/show_deleted_strikeout", true).toBool());
	m_nav_view->set_show_deleted_strikeout(m_record_view->model()->show_deleted_strikeout());
	m_nav_view->set_hide_duplicates(m_hide_duplicates);

	auto nav_header_state = settings.value("view/nav_header").toByteArray();
	if (!nav_header_state.isEmpty())
		m_nav_view->tree_widget()->header()->restoreState(nav_header_state);

	auto main_state = settings.value("session/main_splitter").toByteArray();
	if (!main_state.isEmpty())
		m_main_splitter->restoreState(main_state);

	auto content_state = settings.value("session/content_splitter").toByteArray();
	if (!content_state.isEmpty())
		m_content_splitter->restoreState(content_state);

	m_session->restore_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");

	auto rec_type = settings.value("session/nav_rec_type").toString().toStdString();
	auto record_id = settings.value("session/nav_record_id").toString().toStdString();

	if (rec_type.empty() && record_id.empty())
		return;

	auto target_index = m_nav_view->find_index(rec_type, record_id);
	if (!target_index.isValid())
		return;

	auto parent = m_nav_view->parent_index(target_index);
	if (parent.isValid())
	{
		auto grandparent = m_nav_view->parent_index(parent);
		if (grandparent.isValid())
			m_nav_view->tree_widget()->expand(grandparent);

		m_nav_view->tree_widget()->expand(parent);
	}

	m_nav_view->tree_widget()->setCurrentIndex(target_index);
	m_nav_view->tree_widget()->scrollTo(target_index);
}

void plugin_workspace_view_t::refresh_views()
{
	m_nav_view->tree_widget()->viewport()->update();
	m_record_view->tree()->viewport()->update();
}

void plugin_workspace_view_t::start_lua_scan()
{
	if (m_lua_scan_worker->isRunning())
	{
		m_lua_scan_worker->cancel_scan();
		m_lua_scan_worker->wait();
	}

	const auto & data_paths = m_session->lua_data_paths();
	const auto & mod_names = m_session->lua_mod_names();

	if (data_paths.empty())
		return;

	log_message("[info] starting Lua handler scan...");
	m_status_label->setText(tr("Scanning Lua handlers..."));
	m_lua_scan_worker->start_scan(data_paths, mod_names);
}

void plugin_workspace_view_t::on_lua_scan_complete(const lua_scan_result_t & result)
{
	m_lua_scan_result = result;

	const auto conflict_count = result.conflicts.size();
	const auto reg_count = result.registrations.size();

	log_message(
	    "[info] Lua scan complete: " + std::to_string(reg_count) + " registrations, " + std::to_string(conflict_count) +
	    " conflicts");

	for (const auto & warning : result.warnings)
		log_message("[warning] " + warning);

	m_nav_view->set_lua_scan_result(result);
	update_status();
}
