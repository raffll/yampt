#include "plugin_workspace_view.hpp"
#include "../dialog/filter_dialog.hpp"
#include "../dialog/plugin_select_dialog.hpp"
#include <scanner/cell_name_fixer.hpp>
#include <scanner/fog_fixer.hpp>
#include <scanner/auto_merge.hpp>
#include <scanner/merge_patch_ops.hpp>
#include <scanner/sub_record_merge.hpp>
#include <decoder/view_tree_format.hpp>
#include <scanner/summon_fixer.hpp>
#include <algorithm>
#include <settings_store.hpp>
#include <set>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QSettings>
#include <QShortcut>
#include <QStyledItemDelegate>
#include <QStyleOptionHeader>
#include <QTreeView>
#include <QVBoxLayout>

class grid_delegate_t : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override
	{
		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);

		auto bg = index.data(Qt::BackgroundRole);
		if (bg.isValid())
		{
			painter->fillRect(opt.rect, bg.value<QBrush>());
			opt.backgroundBrush = bg.value<QBrush>();
		}

		QStyledItemDelegate::paint(painter, opt, index);
	}
};

class colored_header_t : public QHeaderView
{
public:
	using QHeaderView::QHeaderView;

protected:
	void paintSection(QPainter * painter, const QRect & rect, int section) const override
	{
		auto fg = model()->headerData(section, Qt::Horizontal, Qt::ForegroundRole);
		if (!fg.isValid())
		{
			QHeaderView::paintSection(painter, rect, section);
			return;
		}

		painter->save();

		QStyleOptionHeader opt;
		initStyleOption(&opt);
		opt.rect = rect;
		opt.section = section;
		opt.text = {};
		style()->drawControl(QStyle::CE_Header, &opt, painter, this);

		auto text = model()->headerData(section, Qt::Horizontal, Qt::DisplayRole).toString();
		auto text_rect = style()->subElementRect(QStyle::SE_HeaderLabel, &opt, this);
		painter->setPen(fg.value<QBrush>().color());
		painter->drawText(text_rect, Qt::AlignVCenter | Qt::AlignLeft, text);

		painter->restore();
	}
};

plugin_workspace_view_t::plugin_workspace_view_t(settings_store_t & settings, QWidget * parent)
    : QWidget(parent)
    , m_settings(settings)
{
	auto * main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(4, 4, 4, 4);

	m_chk_conflicts = new QCheckBox("Conflicts Only", this);
	m_chk_conflicts->setToolTip("Show only conflicting records");
	m_lbl_count = new QLabel(this);

	m_session = new plugin_session_t(this);

	setup_views();

	main_layout->addWidget(m_main_splitter, 1);

	m_status_label = new QLabel(this);

	m_nav_view = new nav_tree_view_t(m_session->scan(), this);
	m_nav_view->set_excluded_plugins(&m_session->excluded_plugins());
	m_nav_view->set_patch_plugins(&m_session->patch_plugins());
	m_content_splitter->insertWidget(0, m_nav_view);
	m_content_splitter->setSizes({ 300, 700 });
	m_record_view = new record_view_t(this);
	m_record_view->model()->set_excluded_plugins(&m_session->excluded_plugins());
	m_record_view->model()->set_patch_plugins(&m_session->patch_plugins());
	m_record_view->model()->set_display_codepage(static_cast<codepage_t>(m_settings.display_codepage()));
	m_content_splitter->insertWidget(1, m_record_view);

	setup_connections();
	m_session->restore_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");
}

void plugin_workspace_view_t::setup_views()
{
	m_main_splitter = new QSplitter(Qt::Vertical, this);
	m_content_splitter = new QSplitter(Qt::Horizontal, m_main_splitter);

	m_bottom_tabs = new QTabWidget(m_main_splitter);
	m_messages = new messages_view_t(m_bottom_tabs);
	m_preview = new preview_view_t(m_bottom_tabs);
	m_bottom_tabs->addTab(m_messages, "Log");
	m_bottom_tabs->addTab(m_preview, "Preview");

	m_main_splitter->addWidget(m_content_splitter);
	m_main_splitter->addWidget(m_bottom_tabs);
	m_main_splitter->setSizes({ 600, 150 });
	m_main_splitter->setChildrenCollapsible(true);
}

void plugin_workspace_view_t::setup_connections()
{
	connect(m_chk_conflicts, &QCheckBox::checkStateChanged, this, &plugin_workspace_view_t::on_filter_changed);

	connect(m_nav_view, &nav_tree_view_t::selection_changed, this, &plugin_workspace_view_t::on_nav_selection_changed);
	connect(m_nav_view, &nav_tree_view_t::context_menu_requested, this, &plugin_workspace_view_t::on_nav_context_menu);

	connect(m_record_view, &record_view_t::context_menu_requested, this, &plugin_workspace_view_t::on_view_context_menu);
	connect(m_record_view, &record_view_t::selection_changed, this, &plugin_workspace_view_t::on_view_selection_changed);

	connect(m_session, &plugin_session_t::plugins_loaded, this, &plugin_workspace_view_t::rebuild_after_load);
	connect(m_session, &plugin_session_t::plugins_unloaded, this, [this]()
	{
		m_record_view->clear();
		m_nav_view->rebuild();
		update_status();
	});
	connect(m_session, &plugin_session_t::log_message, this, &plugin_workspace_view_t::log_message);

	m_record_view->tree()->viewport()->installEventFilter(this);

	auto * copy_shortcut = new QShortcut(QKeySequence::Copy, m_record_view->tree());
	connect(copy_shortcut, &QShortcut::activated, this, &plugin_workspace_view_t::on_view_copy);
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
	QString dir = QFileDialog::getExistingDirectory(this, "Select Data Files Folder", initial_dir);

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
	load_existing_merged_patch();
}

void plugin_workspace_view_t::on_load_mo2_profile()
{
	const auto initial_dir = QString::fromStdString(m_settings.last_directory());
	QString profile_dir = QFileDialog::getExistingDirectory(this, "Select MO2 Profile Folder", initial_dir);

	if (profile_dir.isEmpty())
		return;

	m_session->load_from_mo2_profile(profile_dir);
	m_settings.set_last_directory(profile_dir.toStdString());
}

void plugin_workspace_view_t::on_load_openmw_cfg()
{
	const auto initial_dir = QString::fromStdString(m_settings.last_directory());

	QString cfg_path =
	    QFileDialog::getOpenFileName(this, "Select openmw.cfg", initial_dir, "OpenMW config (openmw.cfg)");

	if (cfg_path.isEmpty())
		return;

	m_session->load_from_openmw_cfg(cfg_path);
	const auto cfg_dir = QFileInfo(cfg_path).absolutePath();
	m_settings.set_last_directory(cfg_dir.toStdString());
}

void plugin_workspace_view_t::rebuild_after_load()
{
	m_nav_view->rebuild();
	update_status();
}

void plugin_workspace_view_t::rebuild_nav_preserving_state()
{
	m_nav_view->rebuild_preserving_state();
}

void plugin_workspace_view_t::on_nav_selection_changed(const nav_tree_model_t::node_info_t & info)
{
	if (info.record_id.empty())
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

void plugin_workspace_view_t::on_filter_changed()
{
	nav_tree_model_t::filter_state_t state;

	if (m_chk_conflicts->isChecked())
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
		dlg_state.filter_itm_only = m_last_filter_state.filter_itm_only;
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
	nav_state.filter_itm_only = state.filter_itm_only;

	m_last_filter_state = nav_state;
	m_filter_active = true;
	m_has_filter_active = true;
	m_last_quick_filter = nav_state;

	m_nav_view->set_filter(nav_state);
	update_status();
}

void plugin_workspace_view_t::on_save_plugin()
{
	if (!m_session->scan().has_merge())
	{
		log_message("[error] no merged patch exists");
		return;
	}

	if (m_session->scan().merge_record_count() == 0)
	{
		log_message("[error] merged patch is empty");
		return;
	}

	auto output_path = resolve_merge_output_path();
	if (output_path.empty())
	{
		const auto initial_dir = QString::fromStdString(m_settings.last_directory());
		const auto selected = QFileDialog::getSaveFileName(this, "Save Merge Plugin", initial_dir, "ESP files (*.esp)");
		if (selected.isEmpty())
			return;

		output_path = selected.toStdString();
	}

	bool ok_author = false;
	auto author = QInputDialog::getText(this, "Plugin Author", "Author:", QLineEdit::Normal, QString(), &ok_author);

	if (!ok_author)
		return;

	bool ok_desc = false;
	auto description =
	    QInputDialog::getText(this, "Plugin Description", "Description:", QLineEdit::Normal, QString(), &ok_desc);

	if (!ok_desc)
		return;

	auto output_dir = QDir(QFileInfo(QString::fromStdString(output_path)).absolutePath());
	output_dir.mkpath(".");

	const bool result = m_session->scan().save_merge(output_path, author.toStdString(), description.toStdString());

	if (result)
	{
		log_message("[info] saved " + output_path + " (" + std::to_string(m_session->scan().merge_record_count()) + " records)");
	}
	else
	{
		log_message("[error] cannot write to " + output_path);
	}
}

void plugin_workspace_view_t::on_unload_all()
{
	m_session->unload_all();
}

std::string plugin_workspace_view_t::resolve_merge_output_path() const
{
	if (m_session->load_base_path().empty())
		return {};

	static const std::string merge_filename = "Merged Patch.esp";

	if (m_session->load_source() == plugin_session_t::load_source_t::mo2_profile)
	{
		auto output_path = QDir::cleanPath(
		    QString::fromStdString(m_session->load_base_path()) + "/../../overwrite/" + QString::fromStdString(merge_filename));
		return output_path.toStdString();
	}

	if (m_session->load_source() == plugin_session_t::load_source_t::openmw_cfg)
	{
		auto output_path = QDir::cleanPath(
		    QString::fromStdString(m_session->load_base_path()) + "/data/" + QString::fromStdString(merge_filename));
		return output_path.toStdString();
	}

	auto output_dir = QDir(QString::fromStdString(m_session->load_base_path()));
	return output_dir.filePath(QString::fromStdString(merge_filename)).toStdString();
}

void plugin_workspace_view_t::save_merged_patch()
{
	const auto output_path = resolve_merge_output_path();
	if (output_path.empty())
	{
		log_message("[error] cannot save merged patch: output path is empty (load_base_path=" + m_session->load_base_path() + ")");
		return;
	}

	auto output_dir = QDir(QFileInfo(QString::fromStdString(output_path)).absolutePath());
	output_dir.mkpath(".");
	const bool saved = m_session->scan().save_merge(output_path, "yEditor", "Auto-generated merged patch");
	if (saved)
		log_message("[info] saved " + output_path + " (" + std::to_string(m_session->scan().merge_record_count()) + " records)");
	else
		log_message("[error] failed to save " + output_path);
}

void plugin_workspace_view_t::load_existing_merged_patch()
{
	const auto path = resolve_merge_output_path();
	if (path.empty())
		return;

	if (!std::filesystem::exists(path))
	{
		log_message("[warning] merged patch not found: " + path);
		return;
	}

	if (m_session->scan().has_merge())
		return;

	auto merge_filename = std::filesystem::path(path).filename().string();

	for (int i = 0; i < static_cast<int>(m_session->scan().plugin_count()); ++i)
	{
		if (m_session->scan().plugin_filename(i) == merge_filename)
		{
			m_session->scan().set_merge_plugin_from_loaded(i);
			m_session->scan().rebuild_conflicts();
			rebuild_after_load();
			log_message("Tagged existing plugin as merge: " + merge_filename);
			return;
		}
	}

	try
	{
		m_session->scan().load_plugin(path);
		const int loaded_idx = static_cast<int>(m_session->scan().plugin_count()) - 1;
		m_session->scan().set_merge_plugin_from_loaded(loaded_idx);
		m_session->scan().rebuild_conflicts();
		rebuild_after_load();
		m_session->save_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");
		log_message("Loaded existing merged patch: " + path);
	}
	catch (const std::exception & e)
	{
		log_message("[error] cannot load merged patch: " + std::string(e.what()));
	}
}

void plugin_workspace_view_t::on_create_merged_patch()
{
	if (m_session->scan().plugin_count() < 2)
	{
		log_message("Need at least 2 plugins loaded to create a merged patch");
		return;
	}

	if (!m_session->scan().has_merge())
		m_session->scan().set_merge_plugin("Merged Patch.esp");

	create_merge_records();
	m_session->scan().rebuild_conflicts();
	rebuild_nav_preserving_state();

	log_message("[info] merge record count: " + std::to_string(m_session->scan().merge_record_count()));
	save_merged_patch();

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

int plugin_workspace_view_t::create_merge_records()
{
	auto pinned_records = m_session->scan().collect_pinned_records();

	merge_config_t config;
	config.excluded_plugins = m_session->excluded_plugins();
	config.patch_plugins = m_session->patch_plugins();
	config.exclusion_pattern = m_settings.merge_exclusion_pattern();
	config.fog_fix_enabled = m_settings.merge_fog_fix_enabled();
	config.summon_fix_enabled = m_settings.merge_summon_fix_enabled();
	config.cell_name_fix_enabled = m_settings.merge_cell_name_fix_enabled();

	auto_merge_t merge(m_session->scan());
	merge.set_config(config);
	const auto counters = merge.execute();

	m_session->scan().restore_pinned_records(pinned_records);

	for (const auto & entry : merge.log_entries())
		log_message(entry.message);

	return counters.three_way + counters.lists + counters.dialogues + counters.fixes;
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
}



bool plugin_workspace_view_t::handle_subrecord_drop(QDropEvent * drop_event)
{
	drop_event->ignore();
	return true;
}

void plugin_workspace_view_t::on_remove_itm()
{
	const auto info = m_nav_view->current_selection();
	if (info.plugin_idx < 0)
		return;

	if (!m_session->scan().has_merge())
	{
		log_message("No merge plugin — create one first");
		return;
	}

	const auto itms = m_session->scan().itm_entries(info.plugin_idx);
	const auto count_before = m_session->scan().merge_record_count();

	for (const auto * entry : itms)
		m_session->scan().remove_from_merge(entry->rec_type, entry->record_id);

	const auto removed = count_before - m_session->scan().merge_record_count();
	if (removed == 0)
		return;

	log_message("Removed " + std::to_string(removed) + " ITM records from merge");
	m_session->scan().rebuild_conflicts();
	rebuild_nav_preserving_state();
	update_status();
}

void plugin_workspace_view_t::on_nav_context_menu(const QPoint & global_pos, const nav_tree_model_t::node_info_t & info)
{
	if (info.plugin_idx < 0)
		return;

	QMenu menu(this);

	bool is_record_node = !info.record_id.empty();
	bool is_file_node = info.rec_type.empty() && info.record_id.empty();
	bool is_merge = m_session->scan().is_merge_plugin(info.plugin_idx);

	if (is_record_node && is_merge)
	{
		menu.addAction(
		    "Remove",
		    [this, info]()
		{
			m_session->scan().remove_from_merge(info.rec_type, info.record_id);
			m_session->scan().rebuild_conflicts();
			rebuild_nav_preserving_state();
			save_merged_patch();
			log_message("Removed " + info.rec_type + ":" + info.record_id + " from merge");
			update_status();
		});
	}

	if (is_record_node && !is_merge && m_session->scan().has_merge())
	{
		menu.addAction(
		    "Copy to Merge",
		    [this, info]()
		{
			for (const auto & v : m_session->scan().find(info.rec_type, info.record_id)->versions)
			{
				if (v.plugin_idx != info.plugin_idx)
					continue;

				m_session->scan().copy_record_to_merge(info.plugin_idx, v.record_index);
				break;
			}

			m_session->scan().rebuild_conflicts();
			rebuild_nav_preserving_state();
			save_merged_patch();
			log_message("Copied " + info.rec_type + ":" + info.record_id + " to merge");
			update_status();
		});
	}

	if (is_file_node && !is_merge)
	{
		const auto & filename = m_session->scan().plugin_filename(info.plugin_idx);
		const bool excluded = m_session->excluded_plugins().count(filename) > 0;
		const bool is_patch = m_session->patch_plugins().count(filename) > 0;

		if (excluded)
		{
			menu.addAction(
			    "Include in Merge",
			    [this, filename]()
			{
				auto excluded_copy = m_session->excluded_plugins();
				excluded_copy.erase(filename);
				m_session->set_excluded_plugins(excluded_copy);
				m_session->save_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");
				rebuild_nav_preserving_state();
			});
		}
		else
		{
			menu.addAction(
			    "Exclude from Merge",
			    [this, filename]()
			{
				auto excluded_copy = m_session->excluded_plugins();
				excluded_copy.insert(filename);
				m_session->set_excluded_plugins(excluded_copy);
				m_session->save_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");
				rebuild_nav_preserving_state();
			});
		}

		if (is_patch)
		{
			menu.addAction(
			    "Unmark as Patch",
			    [this, filename]()
			{
				auto patch_copy = m_session->patch_plugins();
				patch_copy.erase(filename);
				m_session->set_patch_plugins(patch_copy);
				m_session->save_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");
				rebuild_nav_preserving_state();
			});
		}
		else
		{
			menu.addAction(
			    "Mark as Patch",
			    [this, filename]()
			{
				auto patch_copy = m_session->patch_plugins();
				patch_copy.insert(filename);
				m_session->set_patch_plugins(patch_copy);
				m_session->save_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");
				rebuild_nav_preserving_state();
			});
		}
	}

	if (menu.actions().isEmpty())
		return;

	menu.exec(global_pos);
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

	const int col = current.column() - 1;
	if (col < 0 || col >= static_cast<int>(m_record_view->model()->column_plugin_indices().size()))
	{
		m_preview->clear();
		return;
	}

	const auto & visible = m_record_view->model()->rows();
	const int row_idx = current.parent().isValid() ? current.parent().row() : current.row();
	if (row_idx < 0 || row_idx >= static_cast<int>(visible.size()))
	{
		m_preview->clear();
		return;
	}

	const auto & row = visible[row_idx];
	const auto & rec_type = m_record_view->model()->record_type();
	const auto & record_id = m_record_view->model()->record_id();
	const int plugin_idx = m_record_view->model()->column_plugin_indices()[col];

	const auto * entry = m_session->scan().find(rec_type, record_id);
	if (!entry)
	{
		m_preview->clear();
		return;
	}

	const auto & col_indices = m_record_view->model()->col_type_indices();
	int view_occurrence = 0;
	for (int r = 0; r < row_idx; ++r)
	{
		if (visible[r].type == row.type)
			++view_occurrence;
	}

	auto read_full_value = [&](int target_col) -> std::string
	{
		if (target_col < 0 || target_col >= static_cast<int>(col_indices.size()))
			return {};

		auto it_type = col_indices[target_col].find(row.type);
		if (it_type == col_indices[target_col].end())
			return {};

		if (view_occurrence >= static_cast<int>(it_type->second.size()))
			return {};

		if (it_type->second[view_occurrence] == SIZE_MAX)
			return {};

		const int binary_idx = static_cast<int>(it_type->second[view_occurrence]);
		const int target_plugin = m_record_view->model()->column_plugin_indices()[target_col];

		for (const auto & ver : entry->versions)
		{
			if (ver.plugin_idx != target_plugin)
				continue;

			const auto content = m_session->scan().read_record_content(target_plugin, ver.record_index);
			const auto subs = sub_record_merge_t::parse_sub_records(content);
			if (binary_idx >= static_cast<int>(subs.size()))
				return {};

			return format_value_full(
			    subs[binary_idx].data.data(),
			    subs[binary_idx].data.size(),
			    m_record_view->model()->display_codepage());
		}

		return {};
	};

	const auto right_text = read_full_value(col);
	const auto left_text = (col > 0) ? read_full_value(col - 1) : std::string{};

	if (right_text.empty() && left_text.empty())
		m_preview->clear();
	else
		m_preview->show_comparison(left_text, right_text);
}

void plugin_workspace_view_t::on_view_context_menu(const QPoint & global_pos, const QModelIndex & index)
{
	if (!m_session->scan().has_merge())
		return;

	if (!index.isValid())
		return;

	const int col = index.column() - 1;
	if (col < 0 || col >= static_cast<int>(m_record_view->model()->column_plugin_indices().size()))
		return;

	const int plugin_idx = m_record_view->model()->column_plugin_indices()[col];
	const bool is_on_merge = m_session->scan().is_merge_plugin(plugin_idx);
	const auto & rec_type = m_record_view->model()->record_type();
	const auto & record_id = m_record_view->model()->record_id();

	const bool is_field_row = index.parent().isValid();
	const auto & visible = m_record_view->model()->rows();

	int parent_row_idx = -1;
	int child_field_idx = -1;

	if (is_field_row)
	{
		parent_row_idx = index.parent().row();
		child_field_idx = index.row();
	}
	else
	{
		parent_row_idx = index.row();
	}

	if (parent_row_idx < 0 || parent_row_idx >= static_cast<int>(visible.size()))
		return;

	const auto & row = visible[parent_row_idx];
	int view_occurrence = 0;
	for (int r = 0; r < parent_row_idx; ++r)
	{
		if (visible[r].type == row.type)
			++view_occurrence;
	}

	const auto & col_indices = m_record_view->model()->col_type_indices();
	int binary_idx = -1;
	if (col >= 0 && col < static_cast<int>(col_indices.size()))
	{
		auto it_type = col_indices[col].find(row.type);
		if (it_type != col_indices[col].end() && view_occurrence < static_cast<int>(it_type->second.size()) &&
		    it_type->second[view_occurrence] != SIZE_MAX)
		{
			binary_idx = static_cast<int>(it_type->second[view_occurrence]);
		}
	}

	const bool is_group_row = !row.type.empty() && !row.children.empty() && row.size == 0;
	const bool is_sub_record_row = !row.type.empty() && row.children.empty() && binary_idx >= 0;
	const bool is_schema_row = !row.type.empty() && !row.children.empty() && row.size > 0 && binary_idx >= 0;

	const bool is_merge_sub_record = is_on_merge && !row.type.empty() && row.children.empty();
	const bool is_merge_schema_row = is_on_merge && !row.type.empty() && !row.children.empty() && row.size > 0;

	QMenu menu(this);

	if (!is_on_merge && is_sub_record_row && !is_field_row)
	{
		const auto sub_type = row.type;
		menu.addAction(
		    "Copy Sub-Record to Merged Patch",
		    [this, plugin_idx, rec_type, record_id, sub_type, binary_idx]()
		{ copy_sub_record_to_merge(plugin_idx, rec_type, record_id, sub_type, binary_idx); });
	}

	if (!is_on_merge && is_schema_row && !is_field_row)
	{
		const auto sub_type = row.type;
		menu.addAction(
		    "Copy Sub-Record to Merged Patch",
		    [this, plugin_idx, rec_type, record_id, sub_type, binary_idx]()
		{ copy_sub_record_to_merge(plugin_idx, rec_type, record_id, sub_type, binary_idx); });
	}

	if (!is_on_merge && is_group_row && !is_field_row)
	{
		menu.addAction(
		    "Copy Group to Merged Patch",
		    [this, plugin_idx, rec_type, record_id, parent_row_idx]()
		{ copy_group_to_merge(plugin_idx, rec_type, record_id, parent_row_idx); });
	}

	if (!is_on_merge && is_field_row && is_schema_row)
	{
		const auto sub_type = row.type;
		const auto sub_size = row.size;
		menu.addAction(
		    "Copy Field to Merged Patch",
		    [this, plugin_idx, rec_type, record_id, sub_type, sub_size, binary_idx, child_field_idx]()
		{ copy_field_to_merge(plugin_idx, rec_type, record_id, sub_type, sub_size, binary_idx, child_field_idx); });
	}

	if (!is_on_merge && is_field_row && is_group_row)
	{
		if (child_field_idx >= 0 && child_field_idx < static_cast<int>(row.children.size()))
		{
			const auto & child = row.children[child_field_idx];
			const auto child_sub_type = merge_patch_ops_t::extract_sub_type_from_field_name(child.name);
			if (!child_sub_type.empty())
			{
				int group_occurrence = 0;
				for (int r = 0; r < parent_row_idx; ++r)
				{
					if (visible[r].type == row.type)
						++group_occurrence;
				}

				int child_binary_idx = -1;
				if (col >= 0 && col < static_cast<int>(col_indices.size()))
				{
					auto it_type = col_indices[col].find(child_sub_type);
					if (it_type != col_indices[col].end() &&
					    group_occurrence < static_cast<int>(it_type->second.size()) &&
					    it_type->second[group_occurrence] != SIZE_MAX)
					{
						child_binary_idx = static_cast<int>(it_type->second[group_occurrence]);
					}
				}

				if (child_binary_idx >= 0)
				{
					menu.addAction(
					    "Copy Sub-Record to Merged Patch",
					    [this, plugin_idx, rec_type, record_id, child_sub_type, child_binary_idx]()
					{ copy_sub_record_to_merge(plugin_idx, rec_type, record_id, child_sub_type, child_binary_idx); });
				}
			}
		}
	}

	if (is_on_merge && (is_merge_sub_record || is_merge_schema_row) && !is_field_row)
	{
		int merge_binary_idx = binary_idx;
		if (merge_binary_idx < 0)
		{
			const auto * entry = m_session->scan().find(rec_type, record_id);
			if (entry)
			{
				std::string merge_content;
				for (const auto & ver : entry->versions)
				{
					if (m_session->scan().is_merge_plugin(ver.plugin_idx))
					{
						merge_content = m_session->scan().read_record_content(ver.plugin_idx, ver.record_index);
						break;
					}
				}

				if (!merge_content.empty())
				{
					auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
					int occurrence = 0;
					for (size_t s = 0; s < merge_subs.size(); ++s)
					{
						if (merge_subs[s].type == row.type)
						{
							if (occurrence == view_occurrence)
							{
								merge_binary_idx = static_cast<int>(s);
								break;
							}

							++occurrence;
						}
					}
				}
			}
		}

		if (merge_binary_idx >= 0)
		{
			menu.addAction(
			    "Remove Sub-Record",
			    [this, rec_type, record_id, merge_binary_idx]()
			{
				const auto * entry = m_session->scan().find(rec_type, record_id);
				if (!entry)
					return;

				std::string merge_content;
				for (const auto & ver : entry->versions)
				{
					if (m_session->scan().is_merge_plugin(ver.plugin_idx))
					{
						merge_content = m_session->scan().read_record_content(ver.plugin_idx, ver.record_index);
						break;
					}
				}

				if (merge_content.empty())
					return;

				auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
				if (merge_binary_idx >= static_cast<int>(merge_subs.size()))
					return;

				merge_subs.erase(merge_subs.begin() + merge_binary_idx);

				const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
				m_session->scan().copy_record_to_merge_raw(rec_type, record_id, patched);
				log_message("[info] removed sub-record from " + rec_type + ":" + record_id);

				refresh_after_merge(rec_type, record_id);
				save_merged_patch();
			});
		}

	}

	if (is_on_merge && is_group_row && !is_field_row)
	{
		const auto group_type = row.type;
		menu.addAction(
		    "Remove Group",
		    [this, rec_type, record_id, group_type, view_occurrence]()
		{
			const auto * entry = m_session->scan().find(rec_type, record_id);
			if (!entry)
				return;

			std::string merge_content;
			for (const auto & ver : entry->versions)
			{
				if (m_session->scan().is_merge_plugin(ver.plugin_idx))
				{
					merge_content = m_session->scan().read_record_content(ver.plugin_idx, ver.record_index);
					break;
				}
			}

			if (merge_content.empty())
				return;

			auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);

			int occurrence = 0;
			int group_start = -1;
			for (size_t s = 0; s < merge_subs.size(); ++s)
			{
				if (merge_subs[s].type != group_type)
					continue;

				if (occurrence == view_occurrence)
				{
					group_start = static_cast<int>(s);
					break;
				}

				++occurrence;
			}

			if (group_start < 0)
				return;

			int group_end = static_cast<int>(merge_subs.size());
			for (int s = group_start + 1; s < static_cast<int>(merge_subs.size()); ++s)
			{
				if (merge_subs[s].type == group_type)
				{
					group_end = s;
					break;
				}
			}

			merge_subs.erase(merge_subs.begin() + group_start, merge_subs.begin() + group_end);

			const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
			m_session->scan().copy_record_to_merge_raw(rec_type, record_id, patched);
			log_message("[info] removed group from " + rec_type + ":" + record_id);

			refresh_after_merge(rec_type, record_id);
			save_merged_patch();
		});

	}

	if (menu.actions().isEmpty())
		return;

	menu.exec(global_pos);
}

void plugin_workspace_view_t::copy_sub_record_to_merge(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & sub_type,
    int binary_idx)
{
	const auto source_content = read_source_content(plugin_idx, rec_type, record_id);
	if (source_content.empty())
		return;

	const auto merge_content = ensure_merge_record(plugin_idx, rec_type, record_id, source_content);
	if (merge_content.empty())
		return;

	const auto result = merge_patch_ops_t::patch_sub_record(merge_content, source_content, sub_type, binary_idx);
	if (!result.success)
		return;

	m_session->scan().copy_record_to_merge_raw(rec_type, record_id, result.content);
	log_message("[info] patched " + sub_type + " in " + rec_type + ":" + record_id);

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
}

void plugin_workspace_view_t::copy_group_to_merge(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    int group_row_idx)
{
	const auto source_content = read_source_content(plugin_idx, rec_type, record_id);
	if (source_content.empty())
		return;

	const auto merge_content = ensure_merge_record(plugin_idx, rec_type, record_id, source_content);
	if (merge_content.empty())
		return;

	const auto & visible = m_record_view->model()->rows();
	if (group_row_idx < 0 || group_row_idx >= static_cast<int>(visible.size()))
		return;

	const auto & group_row = visible[group_row_idx];

	int group_occurrence = 0;
	for (int r = 0; r < group_row_idx; ++r)
	{
		if (visible[r].type == group_row.type)
			++group_occurrence;
	}

	const auto source_subs = sub_record_merge_t::parse_sub_records(source_content);
	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);

	int source_start = -1;
	int occurrence = 0;
	for (size_t s = 0; s < source_subs.size(); ++s)
	{
		if (source_subs[s].type != group_row.type)
			continue;

		if (occurrence == group_occurrence)
		{
			source_start = static_cast<int>(s);
			break;
		}

		++occurrence;
	}

	if (source_start < 0)
		return;

	int source_end = static_cast<int>(source_subs.size());
	for (int s = source_start + 1; s < static_cast<int>(source_subs.size()); ++s)
	{
		if (source_subs[s].type == group_row.type)
		{
			source_end = s;
			break;
		}
	}

	int merge_start = -1;
	occurrence = 0;
	for (size_t s = 0; s < merge_subs.size(); ++s)
	{
		if (merge_subs[s].type != group_row.type)
			continue;

		if (occurrence == group_occurrence)
		{
			merge_start = static_cast<int>(s);
			break;
		}

		++occurrence;
	}

	sub_record_sequence_t source_group(
	    source_subs.begin() + source_start, source_subs.begin() + source_end);

	if (merge_start >= 0)
	{
		int merge_end = static_cast<int>(merge_subs.size());
		for (int s = merge_start + 1; s < static_cast<int>(merge_subs.size()); ++s)
		{
			if (merge_subs[s].type == group_row.type)
			{
				merge_end = s;
				break;
			}
		}

		merge_subs.erase(merge_subs.begin() + merge_start, merge_subs.begin() + merge_end);
		merge_subs.insert(merge_subs.begin() + merge_start, source_group.begin(), source_group.end());
	}
	else
	{
		merge_subs.insert(merge_subs.end(), source_group.begin(), source_group.end());
	}

	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	m_session->scan().copy_record_to_merge_raw(rec_type, record_id, patched);
	log_message("[info] patched group in " + rec_type + ":" + record_id);

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
}

void plugin_workspace_view_t::copy_field_to_merge(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & sub_type,
    size_t sub_size,
    int binary_idx,
    int field_idx)
{
	const auto source_content = read_source_content(plugin_idx, rec_type, record_id);
	if (source_content.empty())
		return;

	const auto merge_content = ensure_merge_record(plugin_idx, rec_type, record_id, source_content);
	if (merge_content.empty())
		return;

	const auto result = merge_patch_ops_t::patch_field(
	    merge_content, source_content, rec_type, sub_type, sub_size, binary_idx, field_idx);
	if (!result.success)
		return;

	m_session->scan().copy_record_to_merge_raw(rec_type, record_id, result.content);
	log_message("[info] patched field " + result.description + " in " + sub_type + " of " + rec_type + ":" + record_id);

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
}

std::string plugin_workspace_view_t::read_source_content(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id)
{
	const auto * entry = m_session->scan().find(rec_type, record_id);
	if (!entry)
		return {};

	for (const auto & ver : entry->versions)
	{
		if (ver.plugin_idx == plugin_idx)
			return m_session->scan().read_record_content(plugin_idx, ver.record_index);
	}

	return {};
}

std::string plugin_workspace_view_t::ensure_merge_record(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & source_content)
{
	const auto * merge_content_ptr = m_session->scan().find_merge_content(rec_type, record_id);
	if (merge_content_ptr)
		return *merge_content_ptr;

	m_session->scan().copy_record_to_merge_raw(rec_type, record_id, source_content);
	return source_content;
}

int plugin_workspace_view_t::find_plugin_column(int plugin_idx) const
{
	const auto & indices = m_record_view->model()->column_plugin_indices();
	for (int c = 0; c < static_cast<int>(indices.size()); ++c)
	{
		if (indices[c] == plugin_idx)
			return c;
	}

	return -1;
}

bool plugin_workspace_view_t::eventFilter(QObject * obj, QEvent * event)
{
	const bool is_view = (obj == m_record_view->tree()->viewport());
	const bool is_nav = (obj == m_nav_view->tree_widget()->viewport());

	if ((is_view || is_nav) && event->type() == QEvent::DragEnter)
	{
		auto * drag = static_cast<QDragEnterEvent *>(event);
		const bool has_data = drag->mimeData()->hasFormat("application/x-yampt-record") ||
		                      drag->mimeData()->hasFormat("application/x-yampt-subrecord");
		if (has_data && m_session->scan().has_merge())
		{
			drag->acceptProposedAction();
			return true;
		}
	}

	if (is_view && event->type() == QEvent::DragMove)
		return handle_drag_move_view(static_cast<QDragMoveEvent *>(event));

	if (is_nav && event->type() == QEvent::DragMove)
		return handle_drag_move_nav(static_cast<QDragMoveEvent *>(event));

	if (event->type() != QEvent::Drop)
		return QWidget::eventFilter(obj, event);

	auto * drop_event = static_cast<QDropEvent *>(event);
	const bool has_drop_data = drop_event->mimeData()->hasFormat("application/x-yampt-record") ||
	                           drop_event->mimeData()->hasFormat("application/x-yampt-subrecord");
	if (!has_drop_data)
		return QWidget::eventFilter(obj, event);

	if (!m_session->scan().has_merge())
	{
		log_message("[error] create a merged patch first");
		drop_event->ignore();
		return true;
	}

	if (is_view)
		return handle_drop_on_view(drop_event);

	if (is_nav)
		return handle_drop_on_nav(drop_event);

	return QWidget::eventFilter(obj, event);
}

bool plugin_workspace_view_t::handle_drag_move_view(QDragMoveEvent * drag)
{
	drag->ignore();
	return true;
}

bool plugin_workspace_view_t::handle_drag_move_nav(QDragMoveEvent * drag)
{
	if (!drag->mimeData()->hasFormat("application/x-yampt-record") || !m_session->scan().has_merge())
	{
		drag->ignore();
		return true;
	}

	const auto & target = m_nav_view->tree_widget()->indexAt(drag->position().toPoint());
	if (target.isValid() && m_session->scan().is_merge_plugin(m_nav_view->node_at(target).plugin_idx))
	{
		drag->acceptProposedAction();
		return true;
	}

	drag->ignore();
	return true;
}

bool plugin_workspace_view_t::handle_drop_on_view(QDropEvent * drop_event)
{
	const int column = m_record_view->tree()->columnAt(drop_event->position().toPoint().x());
	const int col_idx = column - 1;
	bool dropped_on_merge = false;

	if (col_idx >= 0 && col_idx < static_cast<int>(m_record_view->model()->column_plugin_indices().size()))
	{
		const int plugin_idx = m_record_view->model()->column_plugin_indices()[col_idx];
		dropped_on_merge = m_session->scan().is_merge_plugin(plugin_idx);
	}

	const auto & payload = QString::fromUtf8(drop_event->mimeData()->data("application/x-yampt-record"));
	const auto & parts = payload.split('\t');
	if (parts.size() != 3)
		return false;

	const auto source_plugin = parts[0].toInt();
	const auto & rec_type = parts[1].toStdString();
	const auto & record_id = parts[2].toStdString();

	if (m_session->scan().is_merge_plugin(source_plugin))
	{
		drop_event->ignore();
		return true;
	}

	const auto * entry = m_session->scan().find(rec_type, record_id);
	if (!entry)
		return true;

	for (const auto & version : entry->versions)
	{
		if (version.plugin_idx != source_plugin)
			continue;

		const auto content = m_session->scan().read_record_content(source_plugin, version.record_index);
		if (dropped_on_merge)
		{
			m_session->scan().pin_record_to_merge(rec_type, record_id, content);
			log_message("Pinned " + rec_type + ":" + record_id + " to merge");
		}
		else
		{
			m_session->scan().copy_record_to_merge(source_plugin, version.record_index);
			log_message("Copied " + rec_type + ":" + record_id + " to merge");
		}
		break;
	}

	refresh_after_merge(rec_type, record_id);
	drop_event->accept();
	return true;
}

bool plugin_workspace_view_t::handle_drop_on_nav(QDropEvent * drop_event)
{
	const auto & target_index = m_nav_view->tree_widget()->indexAt(drop_event->position().toPoint());
	if (!target_index.isValid())
		return false;

	const auto & target_info = m_nav_view->node_at(target_index);
	if (!m_session->scan().is_merge_plugin(target_info.plugin_idx))
		return false;

	const auto & payload = QString::fromUtf8(drop_event->mimeData()->data("application/x-yampt-record"));
	const auto & parts = payload.split('\t');
	if (parts.size() != 3)
		return false;

	const auto source_plugin = parts[0].toInt();
	const auto & rec_type = parts[1].toStdString();
	const auto & record_id = parts[2].toStdString();

	if (m_session->scan().is_merge_plugin(source_plugin))
	{
		drop_event->ignore();
		return true;
	}

	const auto * entry = m_session->scan().find(rec_type, record_id);
	if (!entry)
		return true;

	for (const auto & version : entry->versions)
	{
		if (version.plugin_idx != source_plugin)
			continue;

		m_session->scan().copy_record_to_merge(source_plugin, version.record_index);
		log_message("Copied " + rec_type + ":" + record_id + " to merge");
		break;
	}

	refresh_after_merge(rec_type, record_id);
	drop_event->accept();
	return true;
}

void plugin_workspace_view_t::refresh_after_merge(const std::string & rec_type, const std::string & record_id)
{
	m_session->scan().rebuild_conflicts();
	rebuild_nav_preserving_state();

	const auto * updated = m_session->scan().find(rec_type, record_id);
	if (!updated)
	{
		update_status();
		return;
	}

	display_record_in_view(*updated);
	update_status();
}

void plugin_workspace_view_t::update_status()
{
	size_t plugin_count = m_session->scan().plugin_count();
	size_t record_count = m_session->scan().entries().size();
	size_t conflict_count = 0;

	for (const auto & e : m_session->scan().entries())
	{
		if (e.conflict_all == conflict_all_t::conflict)
			++conflict_count;
	}

	m_lbl_count->setText(
	    QString("%1 plugins, %2 records, %3 conflicts").arg(plugin_count).arg(record_count).arg(conflict_count));

	const auto info = m_nav_view->current_selection();
	if (info.plugin_idx >= 0)
	{
		if (!info.record_id.empty())
		{
			m_status_label->setText(QString::fromStdString(info.rec_type + " : " + info.record_id));
		}
		else if (!info.rec_type.empty())
		{
			m_status_label->setText(QString::fromStdString(info.rec_type));
		}
		else
		{
			m_status_label->setText(QString::fromStdString(m_session->scan().plugin_filename(info.plugin_idx)));
		}
	}
	else
	{
		m_status_label->clear();
	}
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

	auto main_state = settings.value("session/main_splitter").toByteArray();
	if (!main_state.isEmpty())
		m_main_splitter->restoreState(main_state);

	auto content_state = settings.value("session/content_splitter").toByteArray();
	if (!content_state.isEmpty())
		m_content_splitter->restoreState(content_state);

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
