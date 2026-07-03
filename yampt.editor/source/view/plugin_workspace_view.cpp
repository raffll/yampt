#include "plugin_workspace_view.hpp"
#include "../dialog/filter_dialog.hpp"
#include "../dialog/plugin_select_dialog.hpp"
#include <io/app_settings.hpp>
#include <scanner/sub_record_merge.hpp>
#include <scanner/fog_fixer.hpp>
#include <scanner/summon_fixer.hpp>
#include <scanner/cell_name_fixer.hpp>
#include <algorithm>
#include <functional>
#include <regex>
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
#include <QTextStream>
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

plugin_workspace_view_t::plugin_workspace_view_t(app_settings_t & settings, QWidget * parent)
    : QWidget(parent)
    , m_settings(settings)
    , m_merge_column_visible(settings.merge_column_visible())
{
	m_scan.set_merge_visible(m_merge_column_visible);
	auto * main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(4, 4, 4, 4);

	m_chk_conflicts = new QCheckBox("Conflicts Only", this);
	m_chk_conflicts->setToolTip("Show only conflicting records");
	m_lbl_count = new QLabel(this);

	setup_views();

	main_layout->addWidget(m_main_splitter, 1);

	m_status_label = new QLabel(this);

	m_nav_model = new nav_tree_model_t(m_scan, this);
	m_view_model = new view_tree_model_t(this);
	m_nav_view->setModel(m_nav_model);
	m_view_view->setModel(m_view_model);
	m_view_view->setHeader(new colored_header_t(Qt::Horizontal, m_view_view));
	m_view_view->header()->setStretchLastSection(true);
	m_view_view->header()->setMinimumSectionSize(120);

	setup_connections();
	load_plugin_paths();
}

void plugin_workspace_view_t::setup_views()
{
	m_main_splitter = new QSplitter(Qt::Vertical, this);
	m_content_splitter = new QSplitter(Qt::Horizontal, m_main_splitter);

	m_nav_view = new QTreeView(m_content_splitter);
	m_nav_view->setDragEnabled(true);
	m_nav_view->setAcceptDrops(true);
	m_nav_view->setDragDropMode(QAbstractItemView::DragDrop);
	m_nav_view->setContextMenuPolicy(Qt::CustomContextMenu);

	m_view_view = new QTreeView(m_content_splitter);
	m_view_view->setSelectionMode(QAbstractItemView::SingleSelection);
	m_view_view->setSelectionBehavior(QAbstractItemView::SelectItems);
	m_view_view->setRootIsDecorated(true);
	m_view_view->setAlternatingRowColors(false);
	m_view_view->setWordWrap(false);
	m_view_view->setUniformRowHeights(true);
	m_view_view->setDragEnabled(true);
	m_view_view->setAcceptDrops(true);
	m_view_view->setDragDropMode(QAbstractItemView::DragDrop);
	m_view_view->setDragDropOverwriteMode(true);
	m_view_view->setDropIndicatorShown(false);
	m_view_view->setDefaultDropAction(Qt::CopyAction);
	m_view_view->setItemDelegate(new grid_delegate_t(m_view_view));
	m_view_view->setContextMenuPolicy(Qt::CustomContextMenu);

	m_content_splitter->addWidget(m_nav_view);
	m_content_splitter->addWidget(m_view_view);
	m_content_splitter->setSizes({ 300, 700 });

	m_messages = new messages_view_t(m_main_splitter);

	m_main_splitter->addWidget(m_content_splitter);
	m_main_splitter->addWidget(m_messages);
	m_main_splitter->setSizes({ 600, 150 });
	m_main_splitter->setChildrenCollapsible(true);
}

void plugin_workspace_view_t::setup_connections()
{
	connect(m_chk_conflicts, &QCheckBox::checkStateChanged, this, &plugin_workspace_view_t::on_filter_changed);

	connect(m_nav_view, &QTreeView::customContextMenuRequested, this, &plugin_workspace_view_t::on_nav_context_menu);
	connect(m_view_view, &QTreeView::customContextMenuRequested, this, &plugin_workspace_view_t::on_view_context_menu);

	connect(
	    m_nav_view->selectionModel(),
	    &QItemSelectionModel::currentChanged,
	    this,
	    &plugin_workspace_view_t::on_nav_selection_changed);

	m_nav_view->viewport()->installEventFilter(this);
	m_view_view->viewport()->installEventFilter(this);

	auto * copy_shortcut = new QShortcut(QKeySequence::Copy, m_view_view);
	connect(copy_shortcut, &QShortcut::activated, this, &plugin_workspace_view_t::on_view_copy);
}

void plugin_workspace_view_t::on_load_plugins()
{
	QStringList files =
	    QFileDialog::getOpenFileNames(this, "Select Plugins", QString(), "Plugin files (*.esm *.esp);;All files (*)");

	if (files.isEmpty())
		return;

	std::vector<std::string> paths;
	for (const auto & f : files)
		paths.push_back(f.toStdString());

	plugin_select_dialog_t dlg(paths, this);
	if (dlg.exec() != QDialog::Accepted)
		return;

	const auto selected = dlg.selected_paths();
	if (selected.empty())
		return;

	m_load_source = load_source_t::folder;
	m_load_base_path = QFileInfo(QString::fromStdString(selected.front())).absolutePath().toStdString();

	for (const auto & path : selected)
	{
		try
		{
			m_scan.load_plugin(path);
			const auto & idx = m_scan.index(static_cast<int>(m_scan.plugin_count()) - 1);
			log_message(
			    "Loaded " + m_scan.plugin_filename(static_cast<int>(m_scan.plugin_count()) - 1) + " (" +
			    std::to_string(idx.entries().size()) + " records indexed)");
		}
		catch (const std::exception & e)
		{
			auto filename = path;
			auto pos = filename.find_last_of("/\\");
			if (pos != std::string::npos)
				filename = filename.substr(pos + 1);

			log_message("Error loading " + filename + ": " + e.what());
		}
	}

	m_scan.rebuild_conflicts();

	const auto & entries = m_scan.entries();
	size_t conflicts = 0;
	size_t overrides = 0;
	size_t identical = 0;
	for (const auto & e : entries)
	{
		if (e.conflict_all == conflict_all_t::conflict)
			++conflicts;
		else if (e.conflict_all == conflict_all_t::override_benign)
			++overrides;

		for (const auto & v : e.versions)
		{
			if (v.status == conflict_this_t::identical_to_master)
				++identical;
		}
	}

	log_message(
	    "Conflict detection complete: " + std::to_string(conflicts) + " conflicts, " + std::to_string(overrides) +
	    " overrides, " + std::to_string(identical) + " identical");

	rebuild_after_load();
	save_plugin_paths();
}

void plugin_workspace_view_t::load_plugins_from_paths(const std::vector<std::string> & paths, const std::string & base_path)
{
	plugin_select_dialog_t dlg(paths, this);
	if (dlg.exec() != QDialog::Accepted)
		return;

	const auto selected = dlg.selected_paths();
	if (selected.empty())
		return;

	m_load_base_path = base_path;
	m_scan = plugin_scan_t();
	m_scan.set_merge_visible(m_merge_column_visible);
	m_view_model->clear();
	m_nav_model->rebuild();

	for (const auto & path : selected)
	{
		try
		{
			m_scan.load_plugin(path);
			const auto & idx = m_scan.index(static_cast<int>(m_scan.plugin_count()) - 1);
			log_message(
			    "Loaded " + m_scan.plugin_filename(static_cast<int>(m_scan.plugin_count()) - 1) + " (" +
			    std::to_string(idx.entries().size()) + " records indexed)");
		}
		catch (const std::exception & e)
		{
			auto filename = path;
			auto pos = filename.find_last_of("/\\");
			if (pos != std::string::npos)
				filename = filename.substr(pos + 1);

			log_message("Error loading " + filename + ": " + e.what());
		}
	}

	m_scan.rebuild_conflicts();
	rebuild_after_load();
	save_plugin_paths();
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
	m_load_source = load_source_t::folder;
	m_settings.set_last_directory(dir.toStdString());
	load_existing_merged_patch();
}

void plugin_workspace_view_t::on_load_mo2_profile()
{
	const auto initial_dir = QString::fromStdString(m_settings.last_directory());
	QString profile_dir = QFileDialog::getExistingDirectory(this, "Select MO2 Profile Folder", initial_dir);

	if (profile_dir.isEmpty())
		return;

	auto paths = parse_mo2_profile(profile_dir);
	if (paths.empty())
		return;

	load_plugins_from_paths(paths, profile_dir.toStdString());
	m_load_source = load_source_t::mo2_profile;
	m_settings.set_last_directory(profile_dir.toStdString());
	load_existing_merged_patch();
}

std::vector<std::string> plugin_workspace_view_t::parse_mo2_profile(const QString & profile_dir)
{
	QString loadorder_path = profile_dir + "/loadorder.txt";
	QFile loadorder_file(loadorder_path);
	if (!loadorder_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		log_message("Cannot open loadorder.txt in " + profile_dir.toStdString());
		return {};
	}

	std::vector<std::string> plugin_names;
	QTextStream stream(&loadorder_file);
	while (!stream.atEnd())
	{
		auto line = stream.readLine().trimmed();
		if (line.isEmpty() || line.startsWith('#'))
			continue;

		if (line.startsWith('*'))
			line = line.mid(1);

		plugin_names.push_back(line.toStdString());
	}
	loadorder_file.close();

	QDir profile(profile_dir);
	QDir mo2_root = profile;
	mo2_root.cdUp();
	mo2_root.cdUp();

	const auto mods_path = mo2_root.absolutePath() + "/mods";

	QString game_data_path;
	QSettings mo2_ini(mo2_root.absolutePath() + "/ModOrganizer.ini", QSettings::IniFormat);
	game_data_path = mo2_ini.value("General/gamePath").toString();
	if (game_data_path.isEmpty())
		game_data_path = mo2_ini.value("Settings/game_path").toString();

	if (game_data_path.isEmpty())
	{
		QFile ini_file(mo2_root.absolutePath() + "/ModOrganizer.ini");
		if (ini_file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream ini_stream(&ini_file);
			while (!ini_stream.atEnd())
			{
				auto ini_line = ini_stream.readLine().trimmed();
				if (ini_line.startsWith("gamePath=") || ini_line.startsWith("gamePath ="))
				{
					game_data_path = ini_line.mid(ini_line.indexOf('=') + 1).trimmed();
					if (game_data_path.startsWith("@ByteArray(") && game_data_path.endsWith(")"))
						game_data_path = game_data_path.mid(11, game_data_path.size() - 12);

					break;
				}
			}
			ini_file.close();
		}
	}

	if (!game_data_path.isEmpty())
	{
		game_data_path.replace('\\', '/');
		if (!game_data_path.endsWith("/Data Files"))
			game_data_path += "/Data Files";
	}

	mo2_resolve_context_t context;

	QString modlist_path = profile_dir + "/modlist.txt";
	QFile modlist_file(modlist_path);
	if (modlist_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream modlist_stream(&modlist_file);
		while (!modlist_stream.atEnd())
		{
			auto mod_line = modlist_stream.readLine().trimmed();
			if (mod_line.startsWith('+'))
				context.enabled_mods.push_back(mod_line.mid(1).toStdString());
		}
		modlist_file.close();
		std::reverse(context.enabled_mods.begin(), context.enabled_mods.end());
	}

	context.mods_path = mods_path;
	context.game_data_path = game_data_path;

	QDir profile_resolve_dir(profile_dir);
	context.merge_path = profile_resolve_dir.absoluteFilePath(QString::fromStdString(m_settings.mo2_merge_path()));

	auto paths = resolve_mo2_plugins(plugin_names, context);

	const auto merge_filename = m_settings.merge_output_path();
	const auto merge_full_path = context.merge_path + "/" + QString::fromStdString(merge_filename);
	if (QFile::exists(merge_full_path))
	{
		const auto merge_std = merge_full_path.toStdString();
		bool already_included = false;
		for (const auto & resolved : paths)
		{
			if (resolved == merge_std)
			{
				already_included = true;
				break;
			}
		}

		if (!already_included)
			paths.push_back(merge_std);
	}

	return paths;
}

std::vector<std::string> plugin_workspace_view_t::resolve_mo2_plugins(
    const std::vector<std::string> & plugin_names,
    const mo2_resolve_context_t & context)
{
	std::vector<std::string> paths;
	for (const auto & name : plugin_names)
	{
		const auto & resolved = resolve_single_mo2_plugin(name, context);
		if (!resolved.empty())
			paths.push_back(resolved);
		else
			log_message("Cannot find: " + name);
	}

	if (paths.empty())
		log_message("No plugins resolved from MO2 profile");

	return paths;
}

std::string plugin_workspace_view_t::resolve_single_mo2_plugin(
    const std::string & plugin_name,
    const mo2_resolve_context_t & context)
{
	for (const auto & mod_name : context.enabled_mods)
	{
		const auto candidate =
		    context.mods_path + "/" + QString::fromStdString(mod_name) + "/" + QString::fromStdString(plugin_name);
		if (QFile::exists(candidate))
			return candidate.toStdString();
	}

	if (!context.merge_path.isEmpty())
	{
		const auto merge_candidate = context.merge_path + "/" + QString::fromStdString(plugin_name);
		if (QFile::exists(merge_candidate))
			return merge_candidate.toStdString();
	}

	const auto game_file = context.game_data_path + "/" + QString::fromStdString(plugin_name);
	if (QFile::exists(game_file))
		return game_file.toStdString();

	return {};
}

void plugin_workspace_view_t::on_load_openmw_cfg()
{
	const auto initial_dir = QString::fromStdString(m_settings.last_directory());

	QString cfg_path =
	    QFileDialog::getOpenFileName(this, "Select openmw.cfg", initial_dir, "OpenMW config (openmw.cfg)");

	if (cfg_path.isEmpty())
		return;

	auto paths = parse_openmw_cfg(cfg_path);
	if (paths.empty())
		return;

	const auto base = QFileInfo(cfg_path).absolutePath().toStdString();
	load_plugins_from_paths(paths, base);
	m_load_source = load_source_t::openmw_cfg;
	m_settings.set_last_directory(base);
	load_existing_merged_patch();
}

std::vector<std::string> plugin_workspace_view_t::parse_openmw_cfg(const QString & cfg_path)
{
	QFile cfg_file(cfg_path);
	if (!cfg_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		log_message("Cannot open " + cfg_path.toStdString());
		return {};
	}

	std::vector<std::string> data_dirs;
	std::vector<std::string> content_names;

	QTextStream stream(&cfg_file);
	while (!stream.atEnd())
	{
		auto line = stream.readLine().trimmed();

		if (line.startsWith("data=") || line.startsWith("data ="))
		{
			auto value = line.mid(line.indexOf('=') + 1).trimmed();
			if (value.startsWith('"') && value.endsWith('"'))
				value = value.mid(1, value.size() - 2);

			data_dirs.push_back(value.toStdString());
		}
		else if (line.startsWith("content=") || line.startsWith("content ="))
		{
			auto value = line.mid(line.indexOf('=') + 1).trimmed();
			content_names.push_back(value.toStdString());
		}
	}
	cfg_file.close();

	return resolve_openmw_content(content_names, data_dirs);
}

std::vector<std::string> plugin_workspace_view_t::resolve_openmw_content(
    const std::vector<std::string> & content_names,
    const std::vector<std::string> & data_dirs)
{
	std::vector<std::string> paths;
	for (const auto & name : content_names)
	{
		const auto & resolved = resolve_single_content(name, data_dirs);
		if (!resolved.empty())
			paths.push_back(resolved);
		else
			log_message("Cannot find: " + name);
	}

	if (paths.empty())
		log_message("No plugins resolved from openmw.cfg");

	return paths;
}

std::string plugin_workspace_view_t::resolve_single_content(
    const std::string & content_name,
    const std::vector<std::string> & data_dirs)
{
	for (auto it_dir = data_dirs.rbegin(); it_dir != data_dirs.rend(); ++it_dir)
	{
		const auto candidate = QString::fromStdString(*it_dir) + "/" + QString::fromStdString(content_name);
		if (QFile::exists(candidate))
			return candidate.toStdString();
	}

	return {};
}

void plugin_workspace_view_t::rebuild_after_load()
{
	m_nav_model->rebuild();
	m_nav_view->setColumnWidth(0, 280);

	update_status();
}

void plugin_workspace_view_t::rebuild_nav_preserving_state()
{
	std::set<std::string> expanded;

	std::function<void(const QModelIndex &, const std::string &)> collect =
	    [&](const QModelIndex & parent, const std::string & path)
	{
		const auto rows = m_nav_model->rowCount(parent);
		for (int i = 0; i < rows; ++i)
		{
			const auto idx = m_nav_model->index(i, 0, parent);
			if (!idx.isValid())
				continue;

			const auto & text = m_nav_model->data(idx, Qt::DisplayRole).toString().toStdString();
			const auto & full_path = path + "/" + text;

			if (!m_nav_view->isExpanded(idx))
				continue;

			expanded.insert(full_path);
			collect(idx, full_path);
		}
	};

	collect(QModelIndex(), "");
	m_nav_model->rebuild();

	std::function<void(const QModelIndex &, const std::string &)> restore =
	    [&](const QModelIndex & parent, const std::string & path)
	{
		const auto rows = m_nav_model->rowCount(parent);
		for (int i = 0; i < rows; ++i)
		{
			const auto idx = m_nav_model->index(i, 0, parent);
			if (!idx.isValid())
				continue;

			const auto & text = m_nav_model->data(idx, Qt::DisplayRole).toString().toStdString();
			const auto & full_path = path + "/" + text;

			if (!expanded.count(full_path))
				continue;

			m_nav_view->expand(idx);
			restore(idx, full_path);
		}
	};

	restore(QModelIndex(), "");
}

void plugin_workspace_view_t::on_nav_selection_changed(const QModelIndex & current)
{
	if (!current.isValid())
	{
		m_view_model->clear();
		update_status();
		return;
	}

	if (current.row() < 0 || current.row() >= m_nav_model->rowCount(current.parent()))
	{
		m_view_model->clear();
		update_status();
		return;
	}

	const auto info = m_nav_model->node_at(current);
	if (info.record_id.empty())
	{
		m_view_model->clear();
		update_status();
		return;
	}

	const auto * entry = m_scan.find(info.rec_type, info.record_id);
	if (!entry)
	{
		m_view_model->clear();
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
		m_nav_model->set_filter(state);
	}
	else
	{
		if (!m_has_filter_active)
			return;

		m_has_filter_active = false;
		m_last_quick_filter = {};
		m_nav_model->clear_filter();
	}

	update_status();
}

void plugin_workspace_view_t::set_hide_duplicates(bool hide)
{
	m_hide_duplicates = hide;
	m_nav_model->set_hide_duplicates(hide);

	auto current = m_nav_view->currentIndex();
	if (!current.isValid())
		return;

	const auto info = m_nav_model->node_at(current);
	if (info.record_id.empty())
		return;

	const auto * entry = m_scan.find(info.rec_type, info.record_id);
	if (entry)
		display_record_in_view(*entry);
}

void plugin_workspace_view_t::set_merge_column_visible(bool visible)
{
	m_merge_column_visible = visible;
	m_settings.set_merge_column_visible(visible);
	m_scan.set_merge_visible(visible);

	if (m_scan.plugin_count() == 0)
		return;

	m_scan.rebuild_conflicts();
	rebuild_nav_preserving_state();
}

void plugin_workspace_view_t::on_advanced_filter()
{
	auto types = m_scan.all_types();
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

	m_nav_model->set_filter(nav_state);
	update_status();
}

void plugin_workspace_view_t::on_save_plugin()
{
	if (!m_scan.has_merge())
	{
		log_message("[error] no merged patch exists");
		return;
	}

	if (m_scan.merge_record_count() == 0)
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
	auto author =
	    QInputDialog::getText(this, "Plugin Author", "Author:", QLineEdit::Normal, QString(), &ok_author);

	if (!ok_author)
		return;

	bool ok_desc = false;
	auto description =
	    QInputDialog::getText(this, "Plugin Description", "Description:", QLineEdit::Normal, QString(), &ok_desc);

	if (!ok_desc)
		return;

	auto output_dir = QDir(QFileInfo(QString::fromStdString(output_path)).absolutePath());
	output_dir.mkpath(".");

	const bool result = m_scan.save_merge(output_path, author.toStdString(), description.toStdString());

	if (result)
	{
		log_message(
		    "[info] saved " + output_path + " (" + std::to_string(m_scan.merge_record_count()) + " records)");
	}
	else
	{
		log_message("[error] cannot write to " + output_path);
	}
}

void plugin_workspace_view_t::on_unload_all()
{
	m_scan = plugin_scan_t();
	m_scan.set_merge_visible(m_merge_column_visible);
	m_view_model->clear();
	m_nav_model->rebuild();
	update_status();
	save_plugin_paths();
	log_message("All plugins unloaded");
}

std::string plugin_workspace_view_t::resolve_merge_output_path() const
{
	if (m_load_base_path.empty())
		return {};

	const auto merge_filename = m_settings.merge_output_path();
	std::string relative_path;

	switch (m_load_source)
	{
	case load_source_t::openmw_cfg:
		relative_path = m_settings.openmw_merge_path();
		break;
	case load_source_t::mo2_profile:
		relative_path = m_settings.mo2_merge_path();
		break;
	case load_source_t::folder:
	case load_source_t::none:
		relative_path = ".";
		break;
	}

	auto output_dir = QDir(QString::fromStdString(m_load_base_path));
	output_dir.cd(QString::fromStdString(relative_path));
	return output_dir.filePath(QString::fromStdString(merge_filename)).toStdString();
}

void plugin_workspace_view_t::load_existing_merged_patch()
{
	const auto path = resolve_merge_output_path();
	if (path.empty())
		return;

	if (!std::filesystem::exists(path))
		return;

	if (m_scan.has_merge())
		return;

	auto merge_filename = std::filesystem::path(path).filename().string();

	for (int i = 0; i < static_cast<int>(m_scan.plugin_count()); ++i)
	{
		if (m_scan.plugin_filename(i) == merge_filename)
		{
			m_scan.set_merge_plugin_from_loaded(i);
			m_scan.rebuild_conflicts();
			rebuild_after_load();
			log_message("Tagged existing plugin as merge: " + merge_filename);
			return;
		}
	}

	try
	{
		m_scan.load_plugin(path);
		const int loaded_idx = static_cast<int>(m_scan.plugin_count()) - 1;
		m_scan.set_merge_plugin_from_loaded(loaded_idx);
		m_scan.rebuild_conflicts();
		rebuild_after_load();
		save_plugin_paths();
		log_message("Loaded existing merged patch: " + path);
	}
	catch (const std::exception & e)
	{
		log_message("Error loading merged patch: " + std::string(e.what()));
	}
}

void plugin_workspace_view_t::on_create_merged_patch()
{
	if (m_scan.plugin_count() < 2)
	{
		log_message("Need at least 2 plugins loaded to create a merged patch");
		return;
	}

	if (!m_scan.has_merge())
		m_scan.set_merge_plugin("Merged Patch.esp");

	create_merge_records();
	m_scan.rebuild_conflicts();
	rebuild_nav_preserving_state();

	log_message("[info] merge record count: " + std::to_string(m_scan.merge_record_count()));

	const auto output_path = resolve_merge_output_path();
	if (output_path.empty())
	{
		log_message("[error] merge output path is empty");
	}
	else
	{
		log_message("[info] saving to: " + output_path);
		auto output_dir = QDir(QFileInfo(QString::fromStdString(output_path)).absolutePath());
		output_dir.mkpath(".");
		bool result = m_scan.save_merge(output_path, "yEditor", "Auto-generated merged patch");
		if (result)
			log_message("[info] saved " + output_path + " (" + std::to_string(m_scan.merge_record_count()) + " records)");
		else
			log_message("[error] failed to save merged patch to " + output_path);
	}

	auto current = m_nav_view->currentIndex();
	if (!current.isValid())
	{
		update_status();
		return;
	}

	const auto info = m_nav_model->node_at(current);
	if (info.record_id.empty())
	{
		update_status();
		return;
	}

	const auto * updated = m_scan.find(info.rec_type, info.record_id);
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
	auto pinned_records = m_scan.collect_pinned_records();

	m_scan.clear_merge_records();

	const auto counters = merge_phase_one();
	const int fixes = merge_phase_two();
	merge_phase_three();

	m_scan.restore_pinned_records(pinned_records);

	const int total = counters.three_way + counters.lists + counters.dialogues + fixes;
	log_message(
	    "[info] merge: " + std::to_string(counters.three_way) + " merged, " +
	    std::to_string(counters.lists) + " lists, " +
	    std::to_string(counters.dialogues) + " dialogues, " +
	    std::to_string(fixes) + " fixes");

	return total;
}

plugin_workspace_view_t::merge_counters_t plugin_workspace_view_t::merge_phase_one()
{
	merge_counters_t counters {};
	const auto & exclusion_pattern = m_settings.merge_exclusion_pattern();
	std::regex exclusion_regex;
	const bool has_exclusion = !exclusion_pattern.empty();

	if (has_exclusion)
	{
		try { exclusion_regex = std::regex(exclusion_pattern, std::regex::icase); }
		catch (...) { log_message("[error] invalid exclusion regex: " + exclusion_pattern); }
	}

	for (const auto & entry : m_scan.entries())
	{
		if (!is_merge_candidate(entry))
			continue;

		if (!m_settings.merge_type_enabled(entry.rec_type))
			continue;

		if (entry.rec_type == "INFO")
			continue;

		if (has_exclusion && std::regex_search(entry.record_id, exclusion_regex))
			continue;

		dispatch_merge_entry(entry, counters);
	}

	return counters;
}

bool plugin_workspace_view_t::is_merge_candidate(const conflict_entry_t & entry) const
{
	if (entry.versions.size() < 2)
		return false;

	if (entry.conflict_all == conflict_all_t::no_conflict)
		return false;

	if (entry.conflict_all == conflict_all_t::only_one)
		return false;

	const auto & winner = entry.versions.back();
	return winner.status != conflict_this_t::identical_to_master;
}

void plugin_workspace_view_t::dispatch_merge_entry(
    const conflict_entry_t & entry,
    merge_counters_t & counters)
{
	const bool is_leveled = (entry.rec_type == "LEVI" || entry.rec_type == "LEVC");
	const bool is_dialogue = (entry.rec_type == "DIAL");

	auto version_contents = build_version_contents(entry);
	if (version_contents.empty())
		return;

	try
	{
		if (is_leveled && version_contents.size() >= 2)
		{
			m_scan.merge_leveled_list(entry);
			++counters.lists;
		}
		else if (is_dialogue && version_contents.size() >= 2)
		{
			m_scan.merge_dialogue(entry);
			++counters.dialogues;
		}
		else if (version_contents.size() >= 3)
		{
			merge_three_way(entry, std::move(version_contents), counters);
		}
	}
	catch (const std::exception & e)
	{
		log_message("[error] merge " + entry.rec_type + " \"" + entry.record_id + "\": " + e.what());
	}
	catch (...)
	{
		log_message("[error] merge " + entry.rec_type + " \"" + entry.record_id + "\"");
	}
}

std::vector<std::string> plugin_workspace_view_t::build_version_contents(
    const conflict_entry_t & entry)
{
	std::vector<std::string> result;
	for (const auto & ver : entry.versions)
	{
		if (m_scan.is_merge_plugin(ver.plugin_idx))
			continue;

		result.push_back(m_scan.read_record_content(ver.plugin_idx, ver.record_index));
	}
	return result;
}

void plugin_workspace_view_t::merge_three_way(
    const conflict_entry_t & entry,
    std::vector<std::string> version_contents,
    merge_counters_t & counters)
{
	merge_input_t input;
	input.rec_type = entry.rec_type;
	input.record_id = entry.record_id;
	input.version_contents = std::move(version_contents);

	const auto result = sub_record_merge_t::merge(input);
	if (!result.changed)
		return;

	m_scan.copy_record_to_merge_raw(entry.rec_type, entry.record_id, result.content);
	++counters.three_way;

	std::string plugins;
	for (const auto & ver : entry.versions)
	{
		if (m_scan.is_merge_plugin(ver.plugin_idx))
			continue;

		if (!plugins.empty())
			plugins += ", ";

		plugins += m_scan.plugin_filename(ver.plugin_idx);
	}
	log_message("[info] merge 3-way: " + entry.rec_type + " \"" + entry.record_id + "\" (" + plugins + ")");
}

int plugin_workspace_view_t::merge_phase_two()
{
	int fixes = 0;

	if (m_settings.merge_fog_fix_enabled())
		fixes += apply_fog_fixes();

	if (m_settings.merge_summon_fix_enabled())
		fixes += apply_summon_fixes();

	if (m_settings.merge_cell_name_fix_enabled())
		fixes += apply_cell_name_fixes();

	return fixes;
}

std::string plugin_workspace_view_t::get_winning_content(
    const std::string & rec_type,
    const std::string & record_id)
{
	const auto * entry = m_scan.find(rec_type, record_id);
	if (!entry || entry->versions.empty())
		return {};

	for (auto it = entry->versions.rbegin(); it != entry->versions.rend(); ++it)
	{
		if (m_scan.is_merge_plugin(it->plugin_idx))
			continue;

		return m_scan.read_record_content(it->plugin_idx, it->record_index);
	}

	return {};
}

std::string plugin_workspace_view_t::get_merge_or_winning_content(
    const std::string & rec_type,
    const std::string & record_id)
{
	const auto * merge_content = m_scan.find_merge_content(rec_type, record_id);
	if (merge_content)
		return *merge_content;

	return get_winning_content(rec_type, record_id);
}

int plugin_workspace_view_t::apply_fog_fixes()
{
	int fixes = 0;
	std::set<std::string> processed;

	for (int pi = static_cast<int>(m_scan.plugin_count()) - 1; pi >= 0; --pi)
	{
		if (m_scan.is_merge_plugin(pi))
			continue;

		const auto & idx = m_scan.index(pi);
		for (const auto & rec : idx.entries())
		{
			if (rec.rec_type != "CELL")
				continue;

			if (processed.count(rec.record_id))
				continue;

			processed.insert(rec.record_id);

			if (m_scan.is_merge_pinned("CELL", rec.record_id))
				continue;

			const auto content = get_merge_or_winning_content("CELL", rec.record_id);
			if (content.empty())
				continue;

			const auto fixed = fog_fixer_t::apply(content);
			if (fixed.empty())
				continue;

			m_scan.copy_record_to_merge_raw("CELL", rec.record_id, fixed);
			log_message("[info] fog fix: \"" + rec.record_id + "\"");
			++fixes;
		}
	}

	return fixes;
}

int plugin_workspace_view_t::apply_summon_fixes()
{
	int fixes = 0;
	std::set<std::string> processed;

	for (int pi = static_cast<int>(m_scan.plugin_count()) - 1; pi >= 0; --pi)
	{
		if (m_scan.is_merge_plugin(pi))
			continue;

		const auto & idx = m_scan.index(pi);
		for (const auto & rec : idx.entries())
		{
			if (rec.rec_type != "CREA")
				continue;

			if (processed.count(rec.record_id))
				continue;

			processed.insert(rec.record_id);

			if (m_scan.is_merge_pinned("CREA", rec.record_id))
				continue;

			const auto content = get_merge_or_winning_content("CREA", rec.record_id);
			if (content.empty())
				continue;

			const auto fixed = summon_fixer_t::apply(rec.record_id, content);
			if (fixed.empty())
				continue;

			m_scan.copy_record_to_merge_raw("CREA", rec.record_id, fixed);
			log_message("[info] summon fix: \"" + rec.record_id + "\"");
			++fixes;
		}
	}

	return fixes;
}

int plugin_workspace_view_t::apply_cell_name_fixes()
{
	int fixes = 0;

	for (const auto & entry : m_scan.entries())
	{
		if (entry.rec_type != "CELL")
			continue;

		if (m_scan.is_merge_pinned("CELL", entry.record_id))
			continue;

		auto version_contents = build_version_contents(entry);
		if (version_contents.size() < 3)
			continue;

		const auto existing = get_merge_or_winning_content("CELL", entry.record_id);
		if (existing.empty())
			continue;

		auto input_versions = version_contents;
		input_versions.back() = existing;

		const auto fixed = cell_name_fixer_t::apply(input_versions);
		if (fixed.empty())
			continue;

		m_scan.copy_record_to_merge_raw("CELL", entry.record_id, fixed);
		log_message("[info] cell name fix: \"" + entry.record_id + "\"");
		++fixes;
	}

	return fixes;
}

void plugin_workspace_view_t::merge_phase_three()
{
	std::vector<std::pair<std::string, std::string>> to_remove;

	for (size_t i = 0; i < m_scan.merge_record_count(); ++i)
	{
		const auto & rec_type = m_scan.merge_record_type(i);
		const auto & record_id = m_scan.merge_record_id(i);
		const auto & merge_content = m_scan.merge_record_content(i);

		const auto winning = get_winning_content(rec_type, record_id);
		if (winning.empty())
			continue;

		if (merge_content == winning)
			to_remove.emplace_back(rec_type, record_id);
	}

	for (const auto & [rec_type, record_id] : to_remove)
		m_scan.remove_from_merge(rec_type, record_id);
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

		m_view_model->set_record(m_scan, filtered);
	}
	else
	{
		m_view_model->set_record(m_scan, entry);
	}

	const auto merge_col = m_view_model->merge_column();
	if (merge_col >= 0)
	{
		const bool should_show = m_scan.has_merge() && m_merge_column_visible;
		m_view_view->header()->setSectionHidden(merge_col, !should_show);
	}

	for (int i = 0; i < m_view_model->rowCount({}); ++i)
	{
		const auto & idx = m_view_model->index(i, 0, {});
		if (m_view_model->rowCount(idx) == 0)
			continue;

		const auto & child = m_view_model->index(0, 0, idx);
		const auto & child_name = child.data(Qt::DisplayRole).toString();
		if (!child_name.isEmpty() && !child_name[0].isDigit())
			m_view_view->expand(idx);
	}

	for (int i = 0; i < m_view_model->columnCount({}); ++i)
		m_view_view->resizeColumnToContents(i);

	const auto total_width = m_view_view->viewport()->width();
	const auto col_count = m_view_model->columnCount({});
	if (col_count <= 1 || total_width <= 0)
		return;

	const auto label_width = std::min(m_view_view->columnWidth(0), total_width / 3);
	const auto remaining = total_width - label_width;
	const auto per_col = remaining / (col_count - 1);

	m_view_view->setColumnWidth(0, label_width);
	for (int i = 1; i < col_count; ++i)
		m_view_view->setColumnWidth(i, per_col);
}

void plugin_workspace_view_t::on_remove_itm()
{
	auto current = m_nav_view->currentIndex();
	if (!current.isValid())
		return;

	auto info = m_nav_model->node_at(current);
	if (info.plugin_idx < 0)
		return;

	if (!m_scan.has_merge())
	{
		log_message("No merge plugin — create one first");
		return;
	}

	const auto itms = m_scan.itm_entries(info.plugin_idx);
	const auto count_before = m_scan.merge_record_count();

	for (const auto * entry : itms)
		m_scan.remove_from_merge(entry->rec_type, entry->record_id);

	const auto removed = count_before - m_scan.merge_record_count();
	if (removed == 0)
		return;

	log_message("Removed " + std::to_string(removed) + " ITM records from merge");
	m_scan.rebuild_conflicts();
	rebuild_nav_preserving_state();
	update_status();
}

void plugin_workspace_view_t::on_nav_context_menu(const QPoint & pos)
{
	auto index = m_nav_view->indexAt(pos);
	if (!index.isValid())
		return;

	auto info = m_nav_model->node_at(index);
	if (info.plugin_idx < 0)
		return;

	QMenu menu(this);

	bool is_record_node = !info.record_id.empty();
	bool is_file_node = info.rec_type.empty() && info.record_id.empty();
	bool is_merge = m_scan.is_merge_plugin(info.plugin_idx);

	if (is_record_node && is_merge)
	{
		menu.addAction(
		    "Remove",
		    [this, info]()
		{
			m_scan.remove_from_merge(info.rec_type, info.record_id);
			m_scan.rebuild_conflicts();
			rebuild_nav_preserving_state();
			log_message("Removed " + info.rec_type + ":" + info.record_id + " from merge");
			update_status();
		});
	}

	if (is_record_node && !is_merge && m_scan.has_merge())
	{
		menu.addAction(
		    "Copy to Merge",
		    [this, info]()
		{
			for (const auto & v : m_scan.find(info.rec_type, info.record_id)->versions)
			{
				if (v.plugin_idx != info.plugin_idx)
					continue;

				m_scan.copy_record_to_merge(info.plugin_idx, v.record_index);
				break;
			}

			m_scan.rebuild_conflicts();
			rebuild_nav_preserving_state();
			log_message("Copied " + info.rec_type + ":" + info.record_id + " to merge");
			update_status();
		});
	}

	if (is_file_node && !is_merge && m_scan.has_merge())
	{
		menu.addAction("Remove ITM from Merge", [this]() { on_remove_itm(); });
	}

	if (menu.actions().isEmpty())
		return;

	menu.exec(m_nav_view->viewport()->mapToGlobal(pos));
}

void plugin_workspace_view_t::on_view_copy()
{
	auto indexes = m_view_view->selectionModel()->selectedIndexes();
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

void plugin_workspace_view_t::on_view_context_menu(const QPoint & pos)
{
	if (!m_scan.has_merge())
		return;

	if (m_view_model->merge_column() < 0)
		return;

	auto index = m_view_view->indexAt(pos);
	if (!index.isValid())
		return;

	if (!m_view_model->is_merge_column(index.column()))
		return;

	QMenu menu(this);
	auto * action = menu.addAction("Remove from merge");
	action->setToolTip("Remove this record from the merged patch");

	connect(action, &QAction::triggered, this, [this]()
	{
		const auto & rec_type = m_view_model->record_type();
		const auto & record_id = m_view_model->record_id();
		if (rec_type.empty() || record_id.empty())
			return;

		m_scan.remove_from_merge(rec_type, record_id);
		m_scan.rebuild_conflicts();
		rebuild_nav_preserving_state();
		log_message("Removed " + rec_type + ":" + record_id + " from merge");
		update_status();
	});

	menu.exec(m_view_view->viewport()->mapToGlobal(pos));
}

bool plugin_workspace_view_t::eventFilter(QObject * obj, QEvent * event)
{
	const bool is_view = (obj == m_view_view->viewport());
	const bool is_nav = (obj == m_nav_view->viewport());

	if ((is_view || is_nav) && event->type() == QEvent::DragEnter)
	{
		auto * drag = static_cast<QDragEnterEvent *>(event);
		if (drag->mimeData()->hasFormat("application/x-yampt-record") && m_scan.has_merge())
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
	if (!drop_event->mimeData()->hasFormat("application/x-yampt-record"))
		return QWidget::eventFilter(obj, event);

	if (!m_scan.has_merge())
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
	if (!drag->mimeData()->hasFormat("application/x-yampt-record") || !m_scan.has_merge())
	{
		drag->ignore();
		return true;
	}

	const int column = m_view_view->columnAt(drag->position().toPoint().x());
	if (m_view_model->is_merge_column(column))
	{
		drag->acceptProposedAction();
		return true;
	}

	drag->ignore();
	return true;
}

bool plugin_workspace_view_t::handle_drag_move_nav(QDragMoveEvent * drag)
{
	if (!drag->mimeData()->hasFormat("application/x-yampt-record") || !m_scan.has_merge())
	{
		drag->ignore();
		return true;
	}

	const auto & target = m_nav_view->indexAt(drag->position().toPoint());
	if (target.isValid() && m_scan.is_merge_plugin(m_nav_model->node_at(target).plugin_idx))
	{
		drag->acceptProposedAction();
		return true;
	}

	drag->ignore();
	return true;
}

bool plugin_workspace_view_t::handle_drop_on_view(QDropEvent * drop_event)
{
	const auto & payload = QString::fromUtf8(drop_event->mimeData()->data("application/x-yampt-record"));
	const auto & parts = payload.split('\t');
	if (parts.size() != 3)
		return false;

	const auto source_plugin = parts[0].toInt();
	const auto & rec_type = parts[1].toStdString();
	const auto & record_id = parts[2].toStdString();

	if (m_scan.is_merge_plugin(source_plugin))
	{
		drop_event->ignore();
		return true;
	}

	const auto * entry = m_scan.find(rec_type, record_id);
	if (!entry)
		return true;

	const int column = m_view_view->columnAt(drop_event->position().toPoint().x());
	const bool dropped_on_merge = m_view_model->is_merge_column(column);

	for (const auto & version : entry->versions)
	{
		if (version.plugin_idx != source_plugin)
			continue;

		const auto content = m_scan.read_record_content(source_plugin, version.record_index);
		if (dropped_on_merge)
		{
			m_scan.pin_record_to_merge(rec_type, record_id, content);
			log_message("Pinned " + rec_type + ":" + record_id + " to merge");
		}
		else
		{
			m_scan.copy_record_to_merge(source_plugin, version.record_index);
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
	const auto & target_index = m_nav_view->indexAt(drop_event->position().toPoint());
	if (!target_index.isValid())
		return false;

	const auto & target_info = m_nav_model->node_at(target_index);
	if (!m_scan.is_merge_plugin(target_info.plugin_idx))
		return false;

	const auto & payload = QString::fromUtf8(drop_event->mimeData()->data("application/x-yampt-record"));
	const auto & parts = payload.split('\t');
	if (parts.size() != 3)
		return false;

	const auto source_plugin = parts[0].toInt();
	const auto & rec_type = parts[1].toStdString();
	const auto & record_id = parts[2].toStdString();

	if (m_scan.is_merge_plugin(source_plugin))
	{
		drop_event->ignore();
		return true;
	}

	const auto * entry = m_scan.find(rec_type, record_id);
	if (!entry)
		return true;

	for (const auto & version : entry->versions)
	{
		if (version.plugin_idx != source_plugin)
			continue;

		m_scan.copy_record_to_merge(source_plugin, version.record_index);
		log_message("Copied " + rec_type + ":" + record_id + " to merge");
		break;
	}

	refresh_after_merge(rec_type, record_id);
	drop_event->accept();
	return true;
}

void plugin_workspace_view_t::refresh_after_merge(const std::string & rec_type, const std::string & record_id)
{
	m_scan.rebuild_conflicts();
	rebuild_nav_preserving_state();

	const auto * updated = m_scan.find(rec_type, record_id);
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
	size_t plugin_count = m_scan.plugin_count();
	size_t record_count = m_scan.entries().size();
	size_t conflict_count = 0;

	for (const auto & e : m_scan.entries())
	{
		if (e.conflict_all == conflict_all_t::conflict)
			++conflict_count;
	}

	m_lbl_count->setText(
	    QString("%1 plugins, %2 records, %3 conflicts").arg(plugin_count).arg(record_count).arg(conflict_count));

	auto current = m_nav_view->currentIndex();
	if (current.isValid())
	{
		auto info = m_nav_model->node_at(current);
		if (!info.record_id.empty())
		{
			m_status_label->setText(QString::fromStdString(info.rec_type + " : " + info.record_id));
		}
		else if (!info.rec_type.empty())
		{
			m_status_label->setText(QString::fromStdString(info.rec_type));
		}
		else if (info.plugin_idx >= 0)
		{
			m_status_label->setText(QString::fromStdString(m_scan.plugin_filename(info.plugin_idx)));
		}
		else
		{
			m_status_label->clear();
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

void plugin_workspace_view_t::save_plugin_paths()
{
	QSettings settings(QCoreApplication::applicationDirPath() + "/yEditor.ini", QSettings::IniFormat);

	QStringList plugin_paths;
	for (int i = 0; i < static_cast<int>(m_scan.plugin_count()); ++i)
		plugin_paths.append(QString::fromStdString(m_scan.plugin_path(i)));

	settings.setValue("session/plugin_paths", plugin_paths);
	settings.setValue("session/load_source", static_cast<int>(m_load_source));
	settings.setValue("session/load_base_path", QString::fromStdString(m_load_base_path));
}

void plugin_workspace_view_t::load_plugin_paths()
{
	QSettings settings(QCoreApplication::applicationDirPath() + "/yEditor.ini", QSettings::IniFormat);

	const auto paths = settings.value("session/plugin_paths").toStringList();
	if (paths.isEmpty())
		return;

	m_load_source = static_cast<load_source_t>(settings.value("session/load_source", 0).toInt());
	m_load_base_path = settings.value("session/load_base_path").toString().toStdString();

	for (const auto & qpath : paths)
	{
		const auto path = qpath.toStdString();
		if (path.empty())
			continue;

		if (!QFile::exists(qpath))
		{
			log_message("[warning] skipping missing plugin: " + path);
			continue;
		}

		auto filename = path;
		auto pos = filename.find_last_of("/\\");
		if (pos != std::string::npos)
			filename = filename.substr(pos + 1);

		try
		{
			m_scan.load_plugin(path);
			const int loaded_idx = static_cast<int>(m_scan.plugin_count()) - 1;

			if (filename == m_settings.merge_output_path())
			{
				m_scan.set_merge_plugin_from_loaded(loaded_idx);
				log_message("Loaded merge plugin: " + filename);
			}
			else
			{
				const auto & idx = m_scan.index(loaded_idx);
				log_message(
				    "Loaded " + m_scan.plugin_filename(loaded_idx) + " (" +
				    std::to_string(idx.entries().size()) + " records indexed)");
			}
		}
		catch (const std::exception & e)
		{
			log_message("[error] loading " + filename + ": " + e.what());
		}
	}

	if (m_scan.plugin_count() == 0)
		return;

	m_scan.rebuild_conflicts();
	rebuild_after_load();
}

void plugin_workspace_view_t::save_session_state()
{
	QSettings settings(QCoreApplication::applicationDirPath() + "/yEditor.ini", QSettings::IniFormat);

	settings.setValue("session/main_splitter", m_main_splitter->saveState());
	settings.setValue("session/content_splitter", m_content_splitter->saveState());

	auto current = m_nav_view->currentIndex();
	if (current.isValid() && m_nav_view->visualRect(current).isValid())
	{
		const auto info = m_nav_model->node_at(current);
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

	auto target_index = m_nav_model->find_index(rec_type, record_id);
	if (!target_index.isValid())
		return;

	auto parent_index = m_nav_model->parent(target_index);
	if (parent_index.isValid())
	{
		auto grandparent = m_nav_model->parent(parent_index);
		if (grandparent.isValid())
			m_nav_view->expand(grandparent);

		m_nav_view->expand(parent_index);
	}

	m_nav_view->setCurrentIndex(target_index);
	m_nav_view->scrollTo(target_index);
}

void plugin_workspace_view_t::refresh_views()
{
	m_nav_view->viewport()->update();
	m_view_view->viewport()->update();
}
