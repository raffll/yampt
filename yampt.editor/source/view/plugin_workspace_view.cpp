#include "plugin_workspace_view.hpp"
#include "../dialog/filter_dialog.hpp"
#include "../dialog/plugin_select_dialog.hpp"
#include "../patcher/patch_builder.hpp"
#include <scanner/cell_name_fixer.hpp>
#include <scanner/fog_fixer.hpp>
#include <scanner/auto_merge.hpp>
#include <scanner/merge_patch_ops.hpp>
#include <scanner/record_conflict.hpp>
#include <scanner/sub_record_merge.hpp>
#include <utility/record_behavior.hpp>
#include <scanner/summon_fixer.hpp>
#include <algorithm>
#include <filesystem>
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
	rebuild_nav_preserving_state();
	on_filter_changed();
	update_status();
}

void plugin_workspace_view_t::rebuild_nav_preserving_state()
{
	m_nav_view->rebuild_preserving_state();
}

void plugin_workspace_view_t::on_nav_selection_changed(const nav_tree_model_t::node_info_t & info)
{
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

	const bool result = save_merge_to_file(output_path, author.toStdString(), description.toStdString());

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

	const auto base = QString::fromStdString(m_session->load_base_path());

	if (m_session->load_source() == plugin_session_t::load_source_t::mo2_profile)
	{
		const auto relative = QString::fromStdString(m_settings.merge_path_mo2());
		return QDir::cleanPath(base + "/" + relative).toStdString();
	}

	if (m_session->load_source() == plugin_session_t::load_source_t::openmw_cfg)
	{
		const auto relative = QString::fromStdString(m_settings.merge_path_openmw());
		return QDir::cleanPath(base + "/" + relative).toStdString();
	}

	const auto relative = QString::fromStdString(m_settings.merge_path_folder());
	return QDir::cleanPath(base + "/" + relative).toStdString();
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
	const bool saved = save_merge_to_file(output_path, "yEditor", "Auto-generated merged patch");
	if (saved)
		log_message("[info] saved " + output_path + " (" + std::to_string(m_session->scan().merge_record_count()) + " records)");
	else
		log_message("[error] failed to save " + output_path);
}

bool plugin_workspace_view_t::save_merge_to_file(
    const std::string & output_path,
    const std::string & author,
    const std::string & description)
{
	auto & scan = m_session->scan();
	auto & builder = m_session->patch_builder();

	if (!scan.has_merge())
		return false;

	if (scan.merge_record_count() == 0)
		return false;

	builder.clear();
	for (size_t i = 0; i < scan.merge_record_count(); ++i)
	{
		if (scan.merge_record_type(i) == "TES3")
			continue;

		builder.add_record_raw(scan.merge_record_type(i), scan.merge_record_id(i), scan.merge_record_content(i));
	}

	std::set<int> contributing_plugins;
	for (const auto & entry : scan.entries())
	{
		bool in_merge = false;
		for (const auto & ver : entry.versions)
		{
			if (scan.is_merge_plugin(ver.plugin_idx))
			{
				in_merge = true;
				break;
			}
		}

		if (!in_merge)
			continue;

		for (const auto & ver : entry.versions)
		{
			if (!scan.is_merge_plugin(ver.plugin_idx))
				contributing_plugins.insert(ver.plugin_idx);
		}
	}

	std::vector<patch_builder_t::master_entry_t> masters;
	for (int i = 0; i < static_cast<int>(scan.plugin_count()); ++i)
	{
		if (scan.is_merge_plugin(i))
			continue;

		if (contributing_plugins.find(i) == contributing_plugins.end())
			continue;

		patch_builder_t::master_entry_t master;
		master.filename = scan.plugin_filename(i);

		try
		{
			master.file_size = std::filesystem::file_size(scan.plugin_path(i));
		}
		catch (...)
		{
			master.file_size = 0;
		}

		masters.push_back(std::move(master));
	}

	const auto merge_filename = std::filesystem::path(output_path).filename().string();
	const bool tes3_is_new = (scan.find_merge_content("TES3", merge_filename) == nullptr);

	const auto header_content = patch_builder_t::build_tes3_header(
	    author, description, builder.record_count(), masters);

	scan.copy_record_to_merge_raw("TES3", merge_filename, header_content);

	const bool saved = builder.save(output_path, author, description, masters);

	if (saved && tes3_is_new)
	{
		scan.recompute_single_conflict("TES3", merge_filename);
		m_nav_view->rebuild_preserving_state();
	}

	return saved;
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

	// ── Node kind ────────────────────────────────────────────────────

	enum class node_kind_t { record_on_merge, record_on_source, file_on_source, other };

	const bool is_merge = m_session->scan().is_merge_plugin(info.plugin_idx);

	const auto node_kind = [&]() -> node_kind_t
	{
		if (!info.record_id.empty() && is_merge)
			return node_kind_t::record_on_merge;

		if (!info.record_id.empty() && !is_merge && m_session->scan().has_merge())
			return node_kind_t::record_on_source;

		if (info.rec_type.empty() && info.record_id.empty() && !is_merge)
			return node_kind_t::file_on_source;

		return node_kind_t::other;
	}();

	// ── Decision table ───────────────────────────────────────────────
	//
	// node_kind         | action
	// ─────────────────────────────────────────────────────────────────
	// record_on_merge   | Remove
	// file_on_source    | Include/Exclude from Merge (toggle)
	// file_on_source    | Mark/Unmark as Patch (toggle)
	// ─────────────────────────────────────────────────────────────────

	QMenu menu(this);

	switch (node_kind)
	{
	case node_kind_t::record_on_merge:
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
		break;
	}

	case node_kind_t::record_on_source:
		break;

	case node_kind_t::file_on_source:
	{
		const auto & filename = m_session->scan().plugin_filename(info.plugin_idx);
		const bool excluded = m_session->excluded_plugins().count(filename) > 0;
		const bool is_patch = m_session->patch_plugins().count(filename) > 0;

		menu.addAction(
		    excluded ? "Include in Merge" : "Exclude from Merge",
		    [this, filename, excluded]()
		{
			auto excluded_copy = m_session->excluded_plugins();
			if (excluded)
				excluded_copy.erase(filename);
			else
				excluded_copy.insert(filename);

			m_session->set_excluded_plugins(excluded_copy);
			m_session->save_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");
			rebuild_nav_preserving_state();
		});

		menu.addAction(
		    is_patch ? "Unmark as Patch" : "Mark as Patch",
		    [this, filename, is_patch]()
		{
			auto patch_copy = m_session->patch_plugins();
			if (is_patch)
				patch_copy.erase(filename);
			else
				patch_copy.insert(filename);

			m_session->set_patch_plugins(patch_copy);
			m_session->save_session_state(QCoreApplication::applicationDirPath() + "/yEditor.ini");
			rebuild_nav_preserving_state();
		});
		break;
	}

	case node_kind_t::other:
		break;
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

	const auto * node = m_record_view->model()->node_from_index(current);
	if (!node)
	{
		m_preview->clear();
		return;
	}

	auto read_node_value = [&](int target_col) -> std::string
	{
		if (target_col < 0 || target_col >= static_cast<int>(node->values.size()))
			return {};

		const auto & value = node->values[target_col];
		if (value == non_existent_value)
			return {};

		return value;
	};

	const auto right_text = read_node_value(col);
	const auto left_text = (col > 0) ? read_node_value(col - 1) : std::string{};

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
	const auto & rec_type = m_record_view->model()->record_type();
	const auto & record_id = m_record_view->model()->record_id();

	const auto * node = m_record_view->model()->node_from_index(index);
	if (!node)
		return;

	const auto & row = *node;
	const bool is_field_row = index.parent().isValid();
	const int parent_row_idx = is_field_row ? index.parent().row() : index.row();

	const auto & visible = m_record_view->model()->rows();
	if (parent_row_idx < 0 || parent_row_idx >= static_cast<int>(visible.size()))
		return;

	auto binary_start = [&](const view_tree_model_t::view_node_t & target) -> int
	{
		if (col < 0 || col >= static_cast<int>(target.binary_ranges.size()))
			return -1;

		return target.binary_ranges[col].start;
	};

	const int bin_idx = binary_start(row);

	// ── Column state ─────────────────────────────────────────────────

	enum class col_state_t { not_in_merge, source, merge };

	const bool is_on_merge = m_session->scan().is_merge_plugin(plugin_idx);
	const bool record_in_merge = m_session->scan().find_merge_content(rec_type, record_id) != nullptr;

	const auto col_state = [&]() -> col_state_t
	{
		if (is_on_merge)
			return col_state_t::merge;

		if (!record_in_merge)
			return col_state_t::not_in_merge;

		return col_state_t::source;
	}();

	// ── Row kind ─────────────────────────────────────────────────────

	enum class row_kind_t { sub_record, schema_record, group, field_of_schema, field_of_group, other };

	const auto kind = [&]() -> row_kind_t
	{
		if (is_field_row && row.type.empty())
			return row_kind_t::field_of_schema;

		if (row.type.empty())
			return row_kind_t::other;

		if (is_field_row && !row.children.empty() && row.size == 0)
			return row_kind_t::field_of_group;

		if (is_field_row)
			return row_kind_t::field_of_schema;

		if (!row.children.empty() && row.size == 0)
			return row_kind_t::group;

		if (!row.children.empty() && row.size > 0 && bin_idx >= 0)
			return row_kind_t::schema_record;

		if (row.children.empty() && bin_idx >= 0)
			return row_kind_t::sub_record;

		return row_kind_t::other;
	}();

	// ── Decision table ───────────────────────────────────────────────
	//
	// col_state     | row_kind        | action
	// ─────────────────────────────────────────────────────────────────
	// not_in_merge  | *               | Copy Record to Merged Patch
	// source        | sub_record      | Copy Sub-Record to Merged Patch
	// source        | schema_record   | Copy Sub-Record to Merged Patch
	// source        | group           | Copy Group to Merged Patch
	// source        | field_of_schema | Copy Field to Merged Patch
	// source        | field_of_group  | Copy Group to Merged Patch
	// merge         | sub_record      | Remove Sub-Record
	// merge         | schema_record   | Remove Sub-Record
	// merge         | group           | Remove Group
	// merge         | field_of_group  | Remove Group
	// merge         | field_of_schema | Remove Sub-Record (parent)
	//
	// field_of_schema includes children with empty type (schema fields)
	// ─────────────────────────────────────────────────────────────────

	QMenu menu(this);

	switch (col_state)
	{
	case col_state_t::not_in_merge:
	{
		const auto * behavior = find_record_behavior(rec_type);

		if (behavior->copy_strategy == copy_strategy_t::header_and_selected_group)
		{
			menu.addAction(
			    "Copy Record to Merged Patch",
			    [this, plugin_idx, rec_type, record_id, index, col]()
			{ copy_cell_record_to_merge(plugin_idx, rec_type, record_id, index, col); });
		}
		else
		{
			menu.addAction(
			    "Copy Record to Merged Patch",
			    [this, plugin_idx, rec_type, record_id]()
			{ copy_whole_record_to_merge(plugin_idx, rec_type, record_id); });
		}

		break;
	}

	case col_state_t::source:
	{
		switch (kind)
		{
		case row_kind_t::sub_record:
		case row_kind_t::schema_record:
		{
			const auto sub_type = row.type;
			menu.addAction(
			    "Copy Sub-Record to Merged Patch",
			    [this, plugin_idx, rec_type, record_id, sub_type, bin_idx]()
			{ copy_sub_record_to_merge(plugin_idx, rec_type, record_id, sub_type, bin_idx); });
			break;
		}

		case row_kind_t::group:
		{
			menu.addAction(
			    "Copy Group to Merged Patch",
			    [this, plugin_idx, rec_type, record_id, parent_row_idx]()
			{ copy_group_to_merge(plugin_idx, rec_type, record_id, parent_row_idx); });
			break;
		}

		case row_kind_t::field_of_schema:
		{
			QModelIndex sub_record_index = index.parent();
			const auto * sub_record_node = m_record_view->model()->node_from_index(sub_record_index);

			while (sub_record_node && sub_record_node->type.empty() && sub_record_index.parent().isValid())
			{
				sub_record_index = sub_record_index.parent();
				sub_record_node = m_record_view->model()->node_from_index(sub_record_index);
			}

			if (!sub_record_node || sub_record_node->type.empty())
				break;

			if (row.schema_field_index < 0)
				break;

			const auto sub_type = sub_record_node->type;
			const auto sub_size = sub_record_node->size;
			const int field_bin = binary_start(*sub_record_node);
			const int child_field_idx = row.schema_field_index;
			menu.addAction(
			    "Copy Field to Merged Patch",
			    [this, plugin_idx, rec_type, record_id, sub_type, sub_size, field_bin, child_field_idx]()
			{ copy_field_to_merge(plugin_idx, rec_type, record_id, sub_type, sub_size, field_bin, child_field_idx); });
			break;
		}

		case row_kind_t::field_of_group:
		{
			menu.addAction(
			    "Copy Group to Merged Patch",
			    [this, plugin_idx, rec_type, record_id, parent_row_idx]()
			{ copy_group_to_merge(plugin_idx, rec_type, record_id, parent_row_idx); });
			break;
		}

		case row_kind_t::other:
			break;
		}

		break;
	}

	case col_state_t::merge:
	{
		switch (kind)
		{
		case row_kind_t::sub_record:
		case row_kind_t::schema_record:
		{
			if (bin_idx >= 0)
			{
				const auto removed_type = row.type;
				menu.addAction(
				    "Remove Sub-Record",
				    [this, rec_type, record_id, bin_idx, removed_type]()
				{ remove_sub_record_from_merge(rec_type, record_id, bin_idx, removed_type); });
			}

			break;
		}

		case row_kind_t::group:
		case row_kind_t::field_of_group:
		{
			auto merge_range = (kind == row_kind_t::field_of_group)
			    ? visible[parent_row_idx].binary_ranges[col]
			    : row.binary_ranges[col];

			if (merge_range.start >= 0)
			{
				menu.addAction(
				    "Remove Group",
				    [this, rec_type, record_id, merge_range]()
				{ remove_group_from_merge(rec_type, record_id, merge_range); });
			}

			break;
		}

		case row_kind_t::field_of_schema:
		{
			QModelIndex sub_record_index = index.parent();
			const auto * sub_record_node = m_record_view->model()->node_from_index(sub_record_index);

			while (sub_record_node && sub_record_node->type.empty() && sub_record_index.parent().isValid())
			{
				sub_record_index = sub_record_index.parent();
				sub_record_node = m_record_view->model()->node_from_index(sub_record_index);
			}

			if (sub_record_node && !sub_record_node->type.empty())
			{
				const int merge_bin = binary_start(*sub_record_node);
				if (merge_bin >= 0)
				{
					const auto removed_type = sub_record_node->type;
					menu.addAction(
					    "Remove Sub-Record",
					    [this, rec_type, record_id, merge_bin, removed_type]()
					{ remove_sub_record_from_merge(rec_type, record_id, merge_bin, removed_type); });
				}
			}

			break;
		}

		case row_kind_t::other:
			break;
		}

		break;
	}
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
	log_message("[info] copied " + sub_type + " from " + m_session->scan().plugin_filename(plugin_idx) + " to merge (" + rec_type + ":" + record_id + ")");

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

	const int col = find_plugin_column(plugin_idx);
	const auto & visible = m_record_view->model()->rows();
	if (group_row_idx < 0 || group_row_idx >= static_cast<int>(visible.size()))
		return;

	const auto & group_row = visible[group_row_idx];

	if (col < 0 || col >= static_cast<int>(group_row.binary_ranges.size()))
		return;

	const auto & source_range = group_row.binary_ranges[col];
	if (source_range.start < 0)
	{
		log_message("[error] copy_group: no binary range for column " + std::to_string(col));
		return;
	}

	const auto source_subs = sub_record_merge_t::parse_sub_records(source_content);
	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);

	if (source_range.end_pos > static_cast<int>(source_subs.size()))
	{
		log_message("[error] copy_group: range exceeds source sub-records");
		return;
	}

	sub_record_sequence_t source_group(
	    source_subs.begin() + source_range.start, source_subs.begin() + source_range.end_pos);

	merge_subs.insert(merge_subs.end(), source_group.begin(), source_group.end());

	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	m_session->scan().copy_record_to_merge_raw(rec_type, record_id, patched);
	log_message("[info] copied group \"" + group_row.label + "\" from " + m_session->scan().plugin_filename(plugin_idx) + " to merge (" + rec_type + ":" + record_id + ")");

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
	log_message("[info] copied field " + result.description + " of " + sub_type + " from " + m_session->scan().plugin_filename(plugin_idx) + " to merge (" + rec_type + ":" + record_id + ")");

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

	const auto header_only = sub_record_merge_t::reconstruct_record(source_content, {});
	m_session->scan().copy_record_to_merge_raw(rec_type, record_id, header_only);
	return header_only;
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

void plugin_workspace_view_t::copy_whole_record_to_merge(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id)
{
	const auto * entry = m_session->scan().find(rec_type, record_id);
	if (!entry)
		return;

	for (const auto & ver : entry->versions)
	{
		if (ver.plugin_idx != plugin_idx)
			continue;

		m_session->scan().copy_record_to_merge(plugin_idx, ver.record_index);
		break;
	}

	log_message("[info] copied record to merge (" + rec_type + ":" + record_id + ")");
	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
}

void plugin_workspace_view_t::copy_cell_record_to_merge(
    int plugin_idx,
    const std::string & rec_type,
    const std::string & record_id,
    const QModelIndex & clicked_index,
    int clicked_col)
{
	const auto source_content = read_source_content(plugin_idx, rec_type, record_id);
	if (source_content.empty())
		return;

	auto partition = sub_record_merge_t::partition_cell(source_content);

	uint32_t selected_frmr = 0;
	bool found_frmr = false;

	QModelIndex walk = clicked_index;
	while (walk.isValid())
	{
		const auto * walk_node = m_record_view->model()->node_from_index(walk);
		if (walk_node && walk_node->type == "FRMR" && walk_node->size == 0)
		{
			if (clicked_col >= 0 &&
			    clicked_col < static_cast<int>(walk_node->values.size()) &&
			    !walk_node->values[clicked_col].empty())
			{
				selected_frmr = static_cast<uint32_t>(std::stoul(walk_node->values[clicked_col]));
				found_frmr = true;
			}

			break;
		}

		walk = walk.parent();
	}

	sub_record_sequence_t output = partition.header;

	if (found_frmr)
	{
		for (const auto & group : partition.groups)
		{
			if (group.frmr_index != selected_frmr)
				continue;

			output.insert(output.end(), group.sub_records.begin(), group.sub_records.end());
			break;
		}
	}

	const auto result = sub_record_merge_t::reconstruct_record(source_content, output);
	m_session->scan().copy_record_to_merge_raw(rec_type, record_id, result);
	log_message("[info] copied CELL record to merge (" + record_id + ")");

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
}

void plugin_workspace_view_t::remove_sub_record_from_merge(
    const std::string & rec_type,
    const std::string & record_id,
    int binary_idx,
    const std::string & removed_type)
{
	const auto * entry = m_session->scan().find(rec_type, record_id);
	if (!entry)
		return;

	std::string merge_content;
	for (const auto & ver : entry->versions)
	{
		if (!m_session->scan().is_merge_plugin(ver.plugin_idx))
			continue;

		merge_content = m_session->scan().read_record_content(ver.plugin_idx, ver.record_index);
		break;
	}

	if (merge_content.empty())
		return;

	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	if (binary_idx >= static_cast<int>(merge_subs.size()))
		return;

	merge_subs.erase(merge_subs.begin() + binary_idx);

	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	m_session->scan().copy_record_to_merge_raw(rec_type, record_id, patched);
	log_message("[info] removed " + removed_type + " from merge (" + rec_type + ":" + record_id + ")");

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
}

void plugin_workspace_view_t::remove_group_from_merge(
    const std::string & rec_type,
    const std::string & record_id,
    view_tree_model_t::binary_range_t range)
{
	const auto * entry = m_session->scan().find(rec_type, record_id);
	if (!entry)
		return;

	std::string merge_content;
	for (const auto & ver : entry->versions)
	{
		if (!m_session->scan().is_merge_plugin(ver.plugin_idx))
			continue;

		merge_content = m_session->scan().read_record_content(ver.plugin_idx, ver.record_index);
		break;
	}

	if (merge_content.empty())
		return;

	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	if (range.end_pos > static_cast<int>(merge_subs.size()))
		return;

	merge_subs.erase(merge_subs.begin() + range.start, merge_subs.begin() + range.end_pos);

	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	m_session->scan().copy_record_to_merge_raw(rec_type, record_id, patched);
	log_message("[info] removed group from merge (" + rec_type + ":" + record_id + ")");

	refresh_after_merge(rec_type, record_id);
	save_merged_patch();
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
	m_session->scan().recompute_single_conflict(rec_type, record_id);
	m_nav_view->refresh_colors();

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
