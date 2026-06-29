#include "plugin_workspace_view.hpp"
#include "../dialog/filter_dialog.hpp"
#include "../dialog/plugin_select_dialog.hpp"
#include <algorithm>
#include <functional>
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

plugin_workspace_view_t::plugin_workspace_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(4, 4, 4, 4);

	setup_toolbar();
	setup_views();

	main_layout->addWidget(btn_load_->parentWidget());
	main_layout->addWidget(main_splitter_, 1);

	status_label_ = new QLabel(this);
	main_layout->addWidget(status_label_);

	nav_model_ = new nav_tree_model_t(scan_, this);
	view_model_ = new view_tree_model_t(this);
	nav_view_->setModel(nav_model_);
	view_view_->setModel(view_model_);
	view_view_->header()->setStretchLastSection(true);
	view_view_->header()->setMinimumSectionSize(120);

	setup_connections();
	load_plugin_paths();
}

void plugin_workspace_view_t::setup_toolbar()
{
	auto * toolbar = new QWidget(this);
	auto * toolbar_layout = new QHBoxLayout(toolbar);
	toolbar_layout->setContentsMargins(0, 0, 0, 0);

	btn_load_ = new QPushButton("Load", toolbar);
	auto * load_menu = new QMenu(btn_load_);
	load_menu->addAction("Open Folder...", this, &plugin_workspace_view_t::on_load_data_files);
	load_menu->addAction("MO2 Profile...", this, &plugin_workspace_view_t::on_load_mo2_profile);
	load_menu->addAction("OpenMW config...", this, &plugin_workspace_view_t::on_load_openmw_cfg);
	btn_load_->setMenu(load_menu);
	btn_load_->setVisible(false);
	btn_new_ = new QPushButton("New", toolbar);
	btn_new_->setVisible(false);
	btn_save_ = new QPushButton("Save", toolbar);
	btn_save_->setVisible(false);
	btn_merge_ = new QPushButton("Create Merged Patch", toolbar);
	btn_merge_->setVisible(false);
	btn_filter_ = new QPushButton("Filter", toolbar);
	btn_filter_->setVisible(false);
	chk_conflicts_ = new QCheckBox("Conflicts Only", toolbar);
	cmb_type_filter_ = new QComboBox(toolbar);
	cmb_type_filter_->addItem("All Types");
	cmb_type_filter_->setVisible(false);
	edt_search_ = new QLineEdit(toolbar);
	edt_search_->setPlaceholderText("Search by ID...");
	edt_search_->setMaximumWidth(200);
	edt_search_->setVisible(false);
	lbl_count_ = new QLabel(toolbar);

	toolbar_layout->addWidget(btn_load_);
	toolbar_layout->addWidget(btn_new_);
	toolbar_layout->addWidget(btn_save_);
	toolbar_layout->addWidget(btn_merge_);
	toolbar_layout->addWidget(btn_filter_);
	toolbar_layout->addWidget(chk_conflicts_);
	toolbar_layout->addWidget(cmb_type_filter_);
	toolbar_layout->addWidget(edt_search_);
	toolbar_layout->addWidget(lbl_count_);
	toolbar_layout->addStretch();
}

void plugin_workspace_view_t::setup_views()
{
	main_splitter_ = new QSplitter(Qt::Vertical, this);
	content_splitter_ = new QSplitter(Qt::Horizontal, main_splitter_);

	nav_view_ = new QTreeView(content_splitter_);
	nav_view_->setDragEnabled(true);
	nav_view_->setAcceptDrops(true);
	nav_view_->setDragDropMode(QAbstractItemView::DragDrop);
	nav_view_->setContextMenuPolicy(Qt::CustomContextMenu);

	view_view_ = new QTreeView(content_splitter_);
	view_view_->setSelectionMode(QAbstractItemView::SingleSelection);
	view_view_->setSelectionBehavior(QAbstractItemView::SelectItems);
	view_view_->setRootIsDecorated(true);
	view_view_->setAlternatingRowColors(false);
	view_view_->setDragEnabled(true);
	view_view_->setAcceptDrops(true);
	view_view_->setDragDropMode(QAbstractItemView::DragDrop);
	view_view_->setDragDropOverwriteMode(true);
	view_view_->setDropIndicatorShown(false);
	view_view_->setDefaultDropAction(Qt::CopyAction);
	view_view_->setItemDelegate(new grid_delegate_t(view_view_));

	content_splitter_->addWidget(nav_view_);
	content_splitter_->addWidget(view_view_);
	content_splitter_->setSizes({ 300, 700 });

	messages_ = new messages_view_t(main_splitter_);

	main_splitter_->addWidget(content_splitter_);
	main_splitter_->addWidget(messages_);
	main_splitter_->setSizes({ 600, 150 });
	main_splitter_->setChildrenCollapsible(true);
}

void plugin_workspace_view_t::setup_connections()
{
	connect(btn_new_, &QPushButton::clicked, this, &plugin_workspace_view_t::on_new_plugin);
	connect(btn_save_, &QPushButton::clicked, this, &plugin_workspace_view_t::on_save_plugin);
	connect(btn_merge_, &QPushButton::clicked, this, &plugin_workspace_view_t::on_create_merged_patch);
	connect(btn_filter_, &QPushButton::clicked, this, &plugin_workspace_view_t::on_advanced_filter);

	connect(chk_conflicts_, &QCheckBox::checkStateChanged, this, &plugin_workspace_view_t::on_filter_changed);
	connect(cmb_type_filter_, &QComboBox::currentIndexChanged, this, &plugin_workspace_view_t::on_filter_changed);
	connect(edt_search_, &QLineEdit::textChanged, this, &plugin_workspace_view_t::on_filter_changed);

	connect(nav_view_, &QTreeView::customContextMenuRequested, this, &plugin_workspace_view_t::on_nav_context_menu);

	connect(
	    nav_view_->selectionModel(),
	    &QItemSelectionModel::currentChanged,
	    this,
	    &plugin_workspace_view_t::on_nav_selection_changed);

	nav_view_->viewport()->installEventFilter(this);
	view_view_->viewport()->installEventFilter(this);

	auto * copy_shortcut = new QShortcut(QKeySequence::Copy, view_view_);
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

	for (const auto & path : selected)
	{
		try
		{
			scan_.load_plugin(path);
			const auto & idx = scan_.index(static_cast<int>(scan_.plugin_count()) - 1);
			log_message(
			    "Loaded " + scan_.plugin_filename(static_cast<int>(scan_.plugin_count()) - 1) + " (" +
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

	scan_.rebuild_conflicts();

	const auto & entries = scan_.entries();
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

void plugin_workspace_view_t::load_plugins_from_paths(const std::vector<std::string> & paths)
{
	plugin_select_dialog_t dlg(paths, this);
	if (dlg.exec() != QDialog::Accepted)
		return;

	const auto selected = dlg.selected_paths();
	if (selected.empty())
		return;

	for (const auto & path : selected)
	{
		try
		{
			scan_.load_plugin(path);
			const auto & idx = scan_.index(static_cast<int>(scan_.plugin_count()) - 1);
			log_message(
			    "Loaded " + scan_.plugin_filename(static_cast<int>(scan_.plugin_count()) - 1) + " (" +
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

	scan_.rebuild_conflicts();
	rebuild_after_load();
	save_plugin_paths();
}

void plugin_workspace_view_t::on_load_data_files()
{
	QString dir = QFileDialog::getExistingDirectory(this, "Select Data Files Folder");

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

	load_plugins_from_paths(paths);
}

void plugin_workspace_view_t::on_load_mo2_profile()
{
	QString profile_dir = QFileDialog::getExistingDirectory(this, "Select MO2 Profile Folder");

	if (profile_dir.isEmpty())
		return;

	auto paths = parse_mo2_profile(profile_dir);
	if (paths.empty())
		return;

	load_plugins_from_paths(paths);
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

	return resolve_mo2_plugins(plugin_names, context);
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

	const auto game_file = context.game_data_path + "/" + QString::fromStdString(plugin_name);
	if (QFile::exists(game_file))
		return game_file.toStdString();

	return {};
}

void plugin_workspace_view_t::on_load_openmw_cfg()
{
	QString cfg_path = QFileDialog::getOpenFileName(this, "Select openmw.cfg", QString(), "OpenMW config (openmw.cfg)");

	if (cfg_path.isEmpty())
		return;

	auto paths = parse_openmw_cfg(cfg_path);
	if (paths.empty())
		return;

	load_plugins_from_paths(paths);
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
	nav_model_->rebuild();
	nav_view_->setColumnWidth(0, 280);

	cmb_type_filter_->blockSignals(true);
	cmb_type_filter_->clear();
	cmb_type_filter_->addItem("All Types");
	for (const auto & t : scan_.all_types())
		cmb_type_filter_->addItem(QString::fromStdString(t));
	cmb_type_filter_->blockSignals(false);

	update_status();
}

void plugin_workspace_view_t::rebuild_nav_preserving_state()
{
	std::set<std::string> expanded;

	std::function<void(const QModelIndex &, const std::string &)> collect =
	    [&](const QModelIndex & parent, const std::string & path)
	{
		const auto rows = nav_model_->rowCount(parent);
		for (int i = 0; i < rows; ++i)
		{
			const auto idx = nav_model_->index(i, 0, parent);
			if (!idx.isValid())
				continue;

			const auto & text = nav_model_->data(idx, Qt::DisplayRole).toString().toStdString();
			const auto & full_path = path + "/" + text;

			if (!nav_view_->isExpanded(idx))
				continue;

			expanded.insert(full_path);
			collect(idx, full_path);
		}
	};

	collect(QModelIndex(), "");
	nav_model_->rebuild();

	std::function<void(const QModelIndex &, const std::string &)> restore =
	    [&](const QModelIndex & parent, const std::string & path)
	{
		const auto rows = nav_model_->rowCount(parent);
		for (int i = 0; i < rows; ++i)
		{
			const auto idx = nav_model_->index(i, 0, parent);
			if (!idx.isValid())
				continue;

			const auto & text = nav_model_->data(idx, Qt::DisplayRole).toString().toStdString();
			const auto & full_path = path + "/" + text;

			if (!expanded.count(full_path))
				continue;

			nav_view_->expand(idx);
			restore(idx, full_path);
		}
	};

	restore(QModelIndex(), "");
}

void plugin_workspace_view_t::on_nav_selection_changed(const QModelIndex & current)
{
	if (!current.isValid())
	{
		view_model_->clear();
		update_status();
		return;
	}

	if (current.row() < 0 || current.row() >= nav_model_->rowCount(current.parent()))
	{
		view_model_->clear();
		update_status();
		return;
	}

	const auto info = nav_model_->node_at(current);
	if (info.record_id.empty())
	{
		view_model_->clear();
		update_status();
		return;
	}

	const auto * entry = scan_.find(info.rec_type, info.record_id);
	if (!entry)
	{
		view_model_->clear();
		update_status();
		return;
	}

	display_record_in_view(*entry);
	update_status();
}

void plugin_workspace_view_t::on_filter_changed()
{
	nav_tree_model_t::filter_state_t state;

	if (chk_conflicts_->isChecked())
	{
		state.filter_conflict_all = true;
		state.conflict_all_set.insert(conflict_all_t::conflict);
		state.conflict_all_set.insert(conflict_all_t::override_benign);
	}

	if (cmb_type_filter_->currentIndex() > 0)
	{
		state.filter_by_type = true;
		state.type_set.insert(cmb_type_filter_->currentText().toStdString());
	}

	auto search_text = edt_search_->text().toStdString();
	if (!search_text.empty())
	{
		state.filter_by_id = true;
		state.id_text = search_text;
		state.filter_by_name = true;
		state.name_text = search_text;
	}

	bool has_any = state.filter_conflict_all || state.filter_by_type || state.filter_by_id || state.filter_by_name;

	if (has_any)
	{
		if (has_filter_active_ && state == last_quick_filter_)
			return;

		has_filter_active_ = true;
		last_quick_filter_ = state;
		nav_model_->set_filter(state);
	}
	else
	{
		if (!has_filter_active_)
			return;

		has_filter_active_ = false;
		last_quick_filter_ = {};
		nav_model_->clear_filter();
	}

	update_status();
}

void plugin_workspace_view_t::on_advanced_filter()
{
	auto types = scan_.all_types();
	filter_dialog_t dlg(types, this);

	if (filter_active_)
	{
		filter_dialog_t::filter_state_t dlg_state;
		dlg_state.filter_conflict_all = last_filter_state_.filter_conflict_all;
		dlg_state.conflict_all_set = last_filter_state_.conflict_all_set;
		dlg_state.filter_conflict_this = last_filter_state_.filter_conflict_this;
		dlg_state.conflict_this_set = last_filter_state_.conflict_this_set;
		dlg_state.filter_by_type = last_filter_state_.filter_by_type;
		dlg_state.type_set = last_filter_state_.type_set;
		dlg_state.filter_by_id = last_filter_state_.filter_by_id;
		dlg_state.id_text = last_filter_state_.id_text;
		dlg_state.filter_by_name = last_filter_state_.filter_by_name;
		dlg_state.name_text = last_filter_state_.name_text;
		dlg_state.filter_deleted = last_filter_state_.filter_deleted;
		dlg_state.filter_itm_only = last_filter_state_.filter_itm_only;
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

	last_filter_state_ = nav_state;
	filter_active_ = true;
	has_filter_active_ = true;
	last_quick_filter_ = nav_state;
	btn_filter_->setStyleSheet("background-color: #FFD700;");

	nav_model_->set_filter(nav_state);
	update_status();
}

void plugin_workspace_view_t::on_new_plugin()
{
	bool ok = false;
	QString filename = QInputDialog::getText(this, "New Plugin", "Filename:", QLineEdit::Normal, QString(), &ok);

	if (!ok || filename.isEmpty())
		return;

	if (!filename.endsWith(".esp", Qt::CaseInsensitive))
		filename += ".esp";

	scan_.set_merge_plugin(filename.toStdString());
	rebuild_nav_preserving_state();
	log_message("Created merge plugin: " + filename.toStdString());
	update_status();
}

void plugin_workspace_view_t::on_save_plugin()
{
	if (!scan_.has_merge())
		return;

	QString path = QFileDialog::getSaveFileName(this, "Save Merge Plugin", QString(), "ESP files (*.esp)");

	if (path.isEmpty())
		return;

	bool ok_author = false;
	QString author = QInputDialog::getText(this, "Plugin Author", "Author:", QLineEdit::Normal, QString(), &ok_author);

	if (!ok_author)
		return;

	bool ok_desc = false;
	QString description =
	    QInputDialog::getText(this, "Plugin Description", "Description:", QLineEdit::Normal, QString(), &ok_desc);

	if (!ok_desc)
		return;

	bool result = scan_.save_merge(path.toStdString(), author.toStdString(), description.toStdString());

	if (result)
	{
		log_message("Saved " + path.toStdString() + " (" + std::to_string(scan_.merge_record_count()) + " records)");
	}
	else
	{
		log_message("Error saving plugin");
	}
}

void plugin_workspace_view_t::on_unload_all()
{
	scan_ = plugin_scan_t();
	view_model_->clear();
	nav_model_->rebuild();
	cmb_type_filter_->clear();
	cmb_type_filter_->addItem("All Types");
	update_status();
	log_message("All plugins unloaded");
}

void plugin_workspace_view_t::on_create_merged_patch()
{
	if (scan_.plugin_count() < 2)
	{
		log_message("Need at least 2 plugins loaded to create a merged patch");
		return;
	}

	if (!scan_.has_merge())
		scan_.set_merge_plugin("Merged Patch.esp");

	create_merge_records();
	scan_.rebuild_conflicts();
	rebuild_nav_preserving_state();

	auto current = nav_view_->currentIndex();
	if (!current.isValid())
	{
		update_status();
		return;
	}

	const auto info = nav_model_->node_at(current);
	if (info.record_id.empty())
	{
		update_status();
		return;
	}

	const auto * updated = scan_.find(info.rec_type, info.record_id);
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
	const auto & entries = scan_.entries();
	int copied = 0;
	int merged_lists = 0;
	int merged_dial = 0;

	for (const auto & entry : entries)
	{
		if (entry.versions.size() < 2)
			continue;

		if (entry.conflict_all == conflict_all_t::no_conflict)
			continue;

		if (entry.conflict_all == conflict_all_t::only_one)
			continue;

		const auto & winner = entry.versions.back();
		if (winner.status == conflict_this_t::identical_to_master)
			continue;

		if (entry.rec_type == "LEVI" || entry.rec_type == "LEVC")
		{
			scan_.merge_leveled_list(entry);
			++merged_lists;
			continue;
		}

		if (entry.rec_type == "DIAL")
		{
			scan_.merge_dialogue(entry);
			++merged_dial;
			continue;
		}

		scan_.copy_record_to_merge(winner.plugin_idx, winner.record_index);
		++copied;
	}

	log_message(
	    "Created merged patch: Merged Patch.esp (" + std::to_string(copied) + " records, " +
	    std::to_string(merged_lists) + " leveled lists, " + std::to_string(merged_dial) + " dialogues)");

	return copied + merged_lists + merged_dial;
}

void plugin_workspace_view_t::display_record_in_view(const conflict_entry_t & entry)
{
	view_model_->set_record(scan_, entry);

	for (int i = 0; i < view_model_->rowCount({}); ++i)
	{
		const auto & idx = view_model_->index(i, 0, {});
		if (view_model_->rowCount(idx) == 0)
			continue;

		const auto & child = view_model_->index(0, 0, idx);
		const auto & child_name = child.data(Qt::DisplayRole).toString();
		if (!child_name.isEmpty() && !child_name[0].isDigit())
			view_view_->expand(idx);
	}

	for (int i = 0; i < view_model_->columnCount({}); ++i)
		view_view_->resizeColumnToContents(i);

	const auto total_width = view_view_->viewport()->width();
	const auto col_count = view_model_->columnCount({});
	if (col_count <= 1 || total_width <= 0)
		return;

	const auto label_width = std::min(view_view_->columnWidth(0), total_width / 3);
	const auto remaining = total_width - label_width;
	const auto per_col = remaining / (col_count - 1);

	view_view_->setColumnWidth(0, label_width);
	for (int i = 1; i < col_count; ++i)
		view_view_->setColumnWidth(i, per_col);
}

void plugin_workspace_view_t::on_remove_itm()
{
	auto current = nav_view_->currentIndex();
	if (!current.isValid())
		return;

	auto info = nav_model_->node_at(current);
	if (info.plugin_idx < 0)
		return;

	if (!scan_.has_merge())
	{
		log_message("No merge plugin — create one first");
		return;
	}

	auto itms = scan_.itm_entries(info.plugin_idx);
	int count = 0;
	for (const auto * entry : itms)
	{
		scan_.remove_from_merge(entry->rec_type, entry->record_id);
		++count;
	}

	log_message("Removed " + std::to_string(count) + " ITM records from merge plugin");
	scan_.rebuild_conflicts();
	rebuild_nav_preserving_state();
	update_status();
}

void plugin_workspace_view_t::on_nav_context_menu(const QPoint & pos)
{
	auto index = nav_view_->indexAt(pos);
	if (!index.isValid())
		return;

	auto info = nav_model_->node_at(index);
	if (info.plugin_idx < 0)
		return;

	QMenu menu(this);

	bool is_record_node = !info.record_id.empty();
	bool is_file_node = info.rec_type.empty() && info.record_id.empty();
	bool is_merge = scan_.is_merge_plugin(info.plugin_idx);

	if (is_record_node && is_merge)
	{
		menu.addAction(
		    "Remove",
		    [this, info]()
		{
			scan_.remove_from_merge(info.rec_type, info.record_id);
			scan_.rebuild_conflicts();
			rebuild_nav_preserving_state();
			log_message("Removed " + info.rec_type + ":" + info.record_id + " from merge");
			update_status();
		});
	}

	if (is_record_node && !is_merge && scan_.has_merge())
	{
		menu.addAction(
		    "Copy to Merge",
		    [this, info]()
		{
			for (const auto & v : scan_.find(info.rec_type, info.record_id)->versions)
			{
				if (v.plugin_idx != info.plugin_idx)
					continue;

				scan_.copy_record_to_merge(info.plugin_idx, v.record_index);
				break;
			}

			scan_.rebuild_conflicts();
			rebuild_nav_preserving_state();
			log_message("Copied " + info.rec_type + ":" + info.record_id + " to merge");
			update_status();
		});
	}

	if (is_file_node && !is_merge && scan_.has_merge())
	{
		menu.addAction("Remove ITM from Merge", [this]() { on_remove_itm(); });
	}

	if (menu.actions().isEmpty())
		return;

	menu.exec(nav_view_->viewport()->mapToGlobal(pos));
}

void plugin_workspace_view_t::on_view_copy()
{
	auto indexes = view_view_->selectionModel()->selectedIndexes();
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

bool plugin_workspace_view_t::eventFilter(QObject * obj, QEvent * event)
{
	if (obj == view_view_->viewport() && event->type() == QEvent::DragEnter)
	{
		auto * drag = static_cast<QDragEnterEvent *>(event);
		if (drag->mimeData()->hasFormat("application/x-yampt-record") && scan_.has_merge())
		{
			drag->acceptProposedAction();
			return true;
		}
	}

	if (obj == view_view_->viewport() && event->type() == QEvent::DragMove)
	{
		auto * drag = static_cast<QDragMoveEvent *>(event);
		if (drag->mimeData()->hasFormat("application/x-yampt-record") && scan_.has_merge())
		{
			drag->acceptProposedAction();
			return true;
		}
	}

	if (event->type() != QEvent::Drop)
		return QWidget::eventFilter(obj, event);

	auto * drop_event = static_cast<QDropEvent *>(event);
	if (!drop_event->mimeData()->hasFormat("application/x-yampt-record"))
		return QWidget::eventFilter(obj, event);

	if (!scan_.has_merge())
		return QWidget::eventFilter(obj, event);

	if (obj == view_view_->viewport())
		return handle_drop_on_view(drop_event);

	if (obj == nav_view_->viewport())
		return handle_drop_on_nav(drop_event);

	return QWidget::eventFilter(obj, event);
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

	const auto * entry = scan_.find(rec_type, record_id);
	if (!entry)
		return true;

	bool already_in_merge = false;
	for (const auto & version : entry->versions)
	{
		if (scan_.is_merge_plugin(version.plugin_idx))
		{
			already_in_merge = true;
			break;
		}
	}

	if (!already_in_merge)
	{
		int best_plugin = -1;
		size_t best_index = 0;
		for (const auto & version : entry->versions)
		{
			if (scan_.is_merge_plugin(version.plugin_idx))
				continue;

			best_plugin = version.plugin_idx;
			best_index = version.record_index;
		}

		if (best_plugin >= 0)
		{
			scan_.copy_record_to_merge(best_plugin, best_index);
			log_message("Copied " + rec_type + ":" + record_id + " to merge (winner)");
		}
	}
	else
	{
		for (const auto & version : entry->versions)
		{
			if (version.plugin_idx != source_plugin)
				continue;

			scan_.copy_record_to_merge(source_plugin, version.record_index);
			log_message("Copied " + rec_type + ":" + record_id + " to merge (override)");
			break;
		}
	}

	refresh_after_merge(rec_type, record_id);
	drop_event->accept();
	return true;
}

bool plugin_workspace_view_t::handle_drop_on_nav(QDropEvent * drop_event)
{
	const auto & target_index = nav_view_->indexAt(drop_event->position().toPoint());
	if (!target_index.isValid())
		return false;

	const auto & target_info = nav_model_->node_at(target_index);
	if (!scan_.is_merge_plugin(target_info.plugin_idx))
		return false;

	const auto & payload = QString::fromUtf8(drop_event->mimeData()->data("application/x-yampt-record"));
	const auto & parts = payload.split('\t');
	if (parts.size() != 3)
		return false;

	const auto source_plugin = parts[0].toInt();
	const auto & rec_type = parts[1].toStdString();
	const auto & record_id = parts[2].toStdString();

	const auto * entry = scan_.find(rec_type, record_id);
	if (!entry)
		return true;

	for (const auto & version : entry->versions)
	{
		if (version.plugin_idx != source_plugin)
			continue;

		scan_.copy_record_to_merge(source_plugin, version.record_index);
		log_message("Copied " + rec_type + ":" + record_id + " to merge (drag)");
		break;
	}

	refresh_after_merge(rec_type, record_id);
	drop_event->accept();
	return true;
}

void plugin_workspace_view_t::refresh_after_merge(const std::string & rec_type, const std::string & record_id)
{
	scan_.rebuild_conflicts();
	rebuild_nav_preserving_state();

	const auto * updated = scan_.find(rec_type, record_id);
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
	size_t plugin_count = scan_.plugin_count();
	size_t record_count = scan_.entries().size();
	size_t conflict_count = 0;

	for (const auto & e : scan_.entries())
	{
		if (e.conflict_all == conflict_all_t::conflict)
			++conflict_count;
	}

	lbl_count_->setText(
	    QString("%1 plugins, %2 records, %3 conflicts").arg(plugin_count).arg(record_count).arg(conflict_count));

	auto current = nav_view_->currentIndex();
	if (current.isValid())
	{
		auto info = nav_model_->node_at(current);
		if (!info.record_id.empty())
		{
			status_label_->setText(QString::fromStdString(info.rec_type + " : " + info.record_id));
		}
		else if (!info.rec_type.empty())
		{
			status_label_->setText(QString::fromStdString(info.rec_type));
		}
		else if (info.plugin_idx >= 0)
		{
			status_label_->setText(QString::fromStdString(scan_.plugin_filename(info.plugin_idx)));
		}
		else
		{
			status_label_->clear();
		}
	}
	else
	{
		status_label_->clear();
	}
}

void plugin_workspace_view_t::log_message(const std::string & msg)
{
	messages_->log(msg);
}

void plugin_workspace_view_t::save_plugin_paths()
{
	QSettings settings(QCoreApplication::applicationDirPath() + "/yEditor.ini", QSettings::IniFormat);

	settings.beginWriteArray("editor/plugins");
	for (int i = 0; i < static_cast<int>(scan_.plugin_count()); ++i)
	{
		settings.setArrayIndex(i);
		settings.setValue("path", QString::fromStdString(scan_.plugin_path(i)));
	}
	settings.endArray();
}

void plugin_workspace_view_t::load_plugin_paths()
{
	QSettings settings(QCoreApplication::applicationDirPath() + "/yEditor.ini", QSettings::IniFormat);

	int count = settings.beginReadArray("editor/plugins");
	if (count == 0)
	{
		settings.endArray();
		return;
	}

	std::vector<std::string> paths;
	for (int i = 0; i < count; ++i)
	{
		settings.setArrayIndex(i);
		auto path = settings.value("path").toString().toStdString();
		if (!path.empty())
			paths.push_back(path);
	}
	settings.endArray();

	for (const auto & path : paths)
	{
		auto filename = path;
		auto pos = filename.find_last_of("/\\");
		if (pos != std::string::npos)
			filename = filename.substr(pos + 1);

		if (filename == "Merged Patch.esp")
		{
			scan_.set_merge_plugin("Merged Patch.esp");
			log_message("Detected merge plugin: Merged Patch.esp");
			continue;
		}

		try
		{
			scan_.load_plugin(path);
			const auto & idx = scan_.index(static_cast<int>(scan_.plugin_count()) - 1);
			log_message(
			    "Loaded " + scan_.plugin_filename(static_cast<int>(scan_.plugin_count()) - 1) + " (" +
			    std::to_string(idx.entries().size()) + " records indexed)");
		}
		catch (const std::exception & e)
		{
			log_message("Error loading " + filename + ": " + e.what());
		}
	}

	if (scan_.plugin_count() == 0)
		return;

	scan_.rebuild_conflicts();
	rebuild_after_load();
}
