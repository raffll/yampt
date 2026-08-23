#include "nav_tree_model.hpp"
#include "editable_column_set.hpp"
#include <io/codepage.hpp>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <theme_system.hpp>
#include <QBrush>
#include <QFont>

static int conflict_this_priority(conflict_this_t conflict)
{
	switch (conflict)
	{
	case conflict_this_t::unknown:
		return 0;
	case conflict_this_t::identical_to_master:
		return 1;
	case conflict_this_t::master:
		return 2;
	case conflict_this_t::override_wins:
		return 3;
	case conflict_this_t::conflict_wins:
		return 4;
	case conflict_this_t::conflict_loses:
		return 5;
	case conflict_this_t::deleted:
		return 0;
	default:
		return 0;
	}
}

static size_t unique_plugin_count(const conflict_entry_t & entry)
{
	std::set<int> plugins;
	for (const auto & version : entry.versions)
		plugins.insert(version.plugin_idx);
	return plugins.size();
}

conflict_this_t nav_tree_model_t::record_foreground_for_plugin(const conflict_entry_t & entry, int plugin_idx) const
{
	if (entry.versions.size() <= 1)
		return conflict_this_t::unknown;

	if (m_filter.hide_duplicates() && unique_plugin_count(entry) <= 1)
		return conflict_this_t::unknown;

	for (const auto & version : entry.versions)
	{
		if (version.plugin_idx != plugin_idx)
			continue;

		return version.status;
	}

	return conflict_this_t::unknown;
}

static int natural_compare(const std::string & a, const std::string & b)
{
	size_t i = 0, j = 0;
	while (i < a.size() && j < b.size())
	{
		bool a_digit = std::isdigit(static_cast<unsigned char>(a[i]));
		bool b_digit = std::isdigit(static_cast<unsigned char>(b[j]));

		if (a_digit && b_digit)
		{
			while (i < a.size() && a[i] == '0')
				++i;
			while (j < b.size() && b[j] == '0')
				++j;

			size_t a_start = i, b_start = j;
			while (i < a.size() && std::isdigit(static_cast<unsigned char>(a[i])))
				++i;
			while (j < b.size() && std::isdigit(static_cast<unsigned char>(b[j])))
				++j;

			size_t a_len = i - a_start;
			size_t b_len = j - b_start;

			if (a_len != b_len)
				return a_len < b_len ? -1 : 1;

			for (size_t k = 0; k < a_len; ++k)
			{
				if (a[a_start + k] != b[b_start + k])
					return a[a_start + k] < b[b_start + k] ? -1 : 1;
			}
		}
		else
		{
			unsigned char ca = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(a[i])));
			unsigned char cb = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(b[j])));

			if (ca != cb)
				return ca < cb ? -1 : 1;

			++i;
			++j;
		}
	}

	if (i < a.size())
		return 1;
	if (j < b.size())
		return -1;
	return 0;
}

static const char * type_to_display_name(const std::string & type)
{
	static const std::map<std::string, const char *> names = {
		{ "ACTI", "Activator" },
		{ "ALCH", "Potion" },
		{ "APPA", "Apparatus" },
		{ "ARMO", "Armor" },
		{ "BODY", "Body Part" },
		{ "BOOK", "Book" },
		{ "BSGN", "Birthsign" },
		{ "CELL", "Cell" },
		{ "CLAS", "Class" },
		{ "CLOT", "Clothing" },
		{ "CONT", "Container" },
		{ "CREA", "Creature" },
		{ "DIAL", "Dialogue" },
		{ "DOOR", "Door" },
		{ "ENCH", "Enchantment" },
		{ "FACT", "Faction" },
		{ "GLOB", "Global" },
		{ "GMST", "Game Setting" },
		{ "INFO", "Dialogue Response" },
		{ "INGR", "Ingredient" },
		{ "LAND", "Landscape" },
		{ "LEVC", "Leveled Creature" },
		{ "LEVI", "Leveled Item" },
		{ "LIGH", "Light" },
		{ "LOCK", "Lockpick" },
		{ "LTEX", "Land Texture" },
		{ "MGEF", "Magic Effect" },
		{ "MISC", "Misc. Item" },
		{ "NPC_", "NPC" },
		{ "PGRD", "Path Grid" },
		{ "PROB", "Probe" },
		{ "RACE", "Race" },
		{ "REGN", "Region" },
		{ "REPA", "Repair Item" },
		{ "SCPT", "Script" },
		{ "SKIL", "Skill" },
		{ "SNDG", "Sound Generator" },
		{ "SOUN", "Sound" },
		{ "SPEL", "Spell" },
		{ "SSCR", "Start Script" },
		{ "STAT", "Static" },
		{ "TES3", "File Header" },
		{ "WEAP", "Weapon" },
	};

	auto it = names.find(type);
	if (it != names.end())
		return it->second;

	return nullptr;
}

nav_tree_model_t::nav_tree_model_t(plugin_scan_t & scan, QObject * parent)
    : QAbstractItemModel(parent)
    , m_scan(scan)
{}

void nav_tree_model_t::rebuild()
{
	beginResetModel();
	build_tree();
	endResetModel();
}

void nav_tree_model_t::refresh_colors()
{
	emit dataChanged(
	    index(0, 0, {}), index(rowCount({}) - 1, columnCount({}) - 1, {}), { Qt::BackgroundRole, Qt::ForegroundRole });
}

void nav_tree_model_t::set_filter(const filter_state_t & state)
{
	m_filter.set_filter(state);
	rebuild();
}

void nav_tree_model_t::clear_filter()
{
	m_filter.clear();
	rebuild();
}

void nav_tree_model_t::set_hide_duplicates(bool hide)
{
	m_filter.set_hide_duplicates(hide);
	emit dataChanged(index(0, 0, {}), index(rowCount({}) - 1, columnCount({}) - 1, {}));
}

void nav_tree_model_t::set_show_deleted_strikeout(bool value)
{
	m_show_deleted_strikeout = value;
	emit dataChanged(index(0, 0, {}), index(rowCount({}) - 1, columnCount({}) - 1, {}), { Qt::FontRole });
}

void nav_tree_model_t::set_display_codepage(codepage_t codepage)
{
	m_display_codepage = codepage;
	rebuild();
}

void nav_tree_model_t::set_excluded_plugins(const std::set<std::string> * excluded)
{
	m_filter.set_excluded_plugins(excluded);
}

void nav_tree_model_t::set_patch_plugins(const std::set<std::string> * patch)
{
	m_filter.set_patch_plugins(patch);
}

void nav_tree_model_t::set_editable_columns(const editable_column_set_t * editable)
{
	m_editable_columns = editable;
}

void nav_tree_model_t::build_tree()
{
	m_tree.clear();

	const auto & entries = m_scan.entries();

	for (int p = 0; p < static_cast<int>(m_scan.plugin_count()); ++p)
	{
		file_node_t file_node;
		file_node.plugin_idx = p;

		std::map<std::string, std::vector<visible_record_t>> type_map;

		for (size_t ei = 0; ei < entries.size(); ++ei)
		{
			const auto & entry = entries[ei];

			const bool has_version = std::any_of(
			    entry.versions.begin(),
			    entry.versions.end(),
			    [p](const auto & version) { return version.plugin_idx == p; });

			if (!has_version)
				continue;

			if (!m_filter.passes(entry, p))
				continue;

			type_map[entry.rec_type].push_back({ ei });
		}

		for (auto & [type, recs] : type_map)
		{
			type_group_t group;
			group.type = type;
			group.records = std::move(recs);
			file_node.groups.push_back(std::move(group));
		}

		if (file_node.groups.empty())
			continue;

		std::sort(
		    file_node.groups.begin(),
		    file_node.groups.end(),
		    [](const type_group_t & a, const type_group_t & b)
		{
			if (a.type == "TES3")
				return true;

			if (b.type == "TES3")
				return false;

			const char * name_a = type_to_display_name(a.type);
			const char * name_b = type_to_display_name(b.type);
			const char * sort_a = name_a ? name_a : a.type.c_str();
			const char * sort_b = name_b ? name_b : b.type.c_str();
			return std::strcmp(sort_a, sort_b) < 0;
		});

		m_tree.push_back(std::move(file_node));
	}

	sort_records();
}

QModelIndex nav_tree_model_t::index(int row, int column, const QModelIndex & parent) const
{
	if (column < 0 || column >= 3)
		return {};

	if (!parent.isValid())
		return index_for_root_level(row, column);

	void * ptr = parent.internalPointer();

	if (ptr == nullptr)
	{
		int parent_row = parent.row();
		if (parent_row < 0 || parent_row >= static_cast<int>(m_tree.size()))
			return {};

		const auto & file_node = m_tree[static_cast<size_t>(parent_row)];
		if (row < 0 || row >= static_cast<int>(file_node.groups.size()))
			return {};

		return createIndex(row, column, const_cast<file_node_t *>(&file_node));
	}

	return index_for_esm_group(ptr, parent.row(), row, column);
}

QModelIndex nav_tree_model_t::parent(const QModelIndex & child) const
{
	if (!child.isValid())
		return {};

	void * ptr = child.internalPointer();

	if (ptr == nullptr)
		return {};

	for (size_t file_idx = 0; file_idx < m_tree.size(); ++file_idx)
	{
		if (ptr == &m_tree[file_idx])
			return createIndex(static_cast<int>(file_idx), 0, nullptr);

		for (size_t group_idx = 0; group_idx < m_tree[file_idx].groups.size(); ++group_idx)
		{
			if (ptr == &m_tree[file_idx].groups[group_idx])
				return createIndex(static_cast<int>(group_idx), 0, const_cast<file_node_t *>(&m_tree[file_idx]));
		}
	}

	return {};
}

int nav_tree_model_t::rowCount(const QModelIndex & parent) const
{
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		return static_cast<int>(m_tree.size());

	void * ptr = parent.internalPointer();

	if (ptr == nullptr)
	{
		int file_idx = parent.row();
		if (file_idx < 0 || file_idx >= static_cast<int>(m_tree.size()))
			return 0;

		return static_cast<int>(m_tree[static_cast<size_t>(file_idx)].groups.size());
	}

	for (size_t file_idx = 0; file_idx < m_tree.size(); ++file_idx)
	{
		if (ptr != &m_tree[file_idx])
			continue;

		int group_idx = parent.row();
		if (group_idx < 0 || group_idx >= static_cast<int>(m_tree[file_idx].groups.size()))
			return 0;

		const auto & group = m_tree[file_idx].groups[static_cast<size_t>(group_idx)];
		if (group.type == "TES3")
			return 0;

		return static_cast<int>(group.records.size());
	}

	return 0;
}

int nav_tree_model_t::columnCount(const QModelIndex &) const
{
	return 2;
}

QVariant nav_tree_model_t::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	switch (section)
	{
	case 0:
		return QStringLiteral("ID");
	case 1:
		return QStringLiteral("Name");
	}

	return {};
}

QVariant nav_tree_model_t::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return {};

	void * ptr = index.internalPointer();
	int column = index.column();

	if (ptr == nullptr)
		return data_for_root_level(index.row(), column, role);

	return data_for_esm_nodes(ptr, index.row(), column, role);
}

Qt::ItemFlags nav_tree_model_t::flags(const QModelIndex & index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	Qt::ItemFlags base_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	void * ptr = index.internalPointer();

	for (size_t file_idx = 0; file_idx < m_tree.size(); ++file_idx)
	{
		for (size_t group_idx = 0; group_idx < m_tree[file_idx].groups.size(); ++group_idx)
		{
			if (ptr == &m_tree[file_idx].groups[group_idx])
				return base_flags | Qt::ItemIsDragEnabled;
		}
	}

	return base_flags;
}

Qt::DropActions nav_tree_model_t::supportedDragActions() const
{
	return Qt::CopyAction;
}

QMimeData * nav_tree_model_t::mimeData(const QModelIndexList & indexes) const
{
	if (indexes.isEmpty())
		return nullptr;

	const auto & idx = indexes.first();
	auto info = node_at(idx);

	if (info.record_id.empty())
		return nullptr;

	auto * mime = new QMimeData;
	QString payload = QString("%1\t%2\t%3")
	                      .arg(info.plugin_idx)
	                      .arg(QString::fromStdString(info.rec_type))
	                      .arg(QString::fromStdString(info.record_id));
	mime->setData("application/x-yampt-record", payload.toUtf8());
	return mime;
}

QModelIndex nav_tree_model_t::find_index(const std::string & rec_type, const std::string & record_id) const
{
	if (rec_type.empty() || record_id.empty())
		return {};

	const auto & entries = m_scan.entries();

	for (size_t file_idx = 0; file_idx < m_tree.size(); ++file_idx)
	{
		for (size_t group_idx = 0; group_idx < m_tree[file_idx].groups.size(); ++group_idx)
		{
			const auto & group = m_tree[file_idx].groups[group_idx];
			if (group.type != rec_type)
				continue;

			for (size_t record_idx = 0; record_idx < group.records.size(); ++record_idx)
			{
				const auto & entry = entries[group.records[record_idx].entry_idx];
				if (entry.record_id != record_id)
					continue;

				return createIndex(static_cast<int>(record_idx), 0, const_cast<type_group_t *>(&group));
			}
		}
	}

	return {};
}

nav_tree_model_t::node_info_t nav_tree_model_t::node_at(const QModelIndex & index) const
{
	if (!index.isValid())
		return { -1, {}, {} };

	void * ptr = index.internalPointer();
	const auto & entries = m_scan.entries();

	if (ptr == nullptr)
	{
		int file_idx = index.row();
		if (file_idx < 0 || file_idx >= static_cast<int>(m_tree.size()))
			return { -1, {}, {} };

		return { m_tree[static_cast<size_t>(file_idx)].plugin_idx, {}, {} };
	}

	for (size_t file_idx = 0; file_idx < m_tree.size(); ++file_idx)
	{
		if (ptr == &m_tree[file_idx])
		{
			int group_idx = index.row();
			if (group_idx < 0 || group_idx >= static_cast<int>(m_tree[file_idx].groups.size()))
				return { m_tree[file_idx].plugin_idx, {}, {} };

			const auto & group = m_tree[file_idx].groups[static_cast<size_t>(group_idx)];

			if (group.type == "TES3" && !group.records.empty())
			{
				const auto & entry = entries[group.records[0].entry_idx];
				return { m_tree[file_idx].plugin_idx, entry.rec_type, entry.record_id };
			}

			return { m_tree[file_idx].plugin_idx, group.type, {} };
		}

		for (size_t group_idx = 0; group_idx < m_tree[file_idx].groups.size(); ++group_idx)
		{
			if (ptr != &m_tree[file_idx].groups[group_idx])
				continue;

			int rec_idx = index.row();
			if (rec_idx < 0 || rec_idx >= static_cast<int>(m_tree[file_idx].groups[group_idx].records.size()))
				return { m_tree[file_idx].plugin_idx, m_tree[file_idx].groups[group_idx].type, {} };

			const auto & vis = m_tree[file_idx].groups[group_idx].records[static_cast<size_t>(rec_idx)];
			const auto & entry = entries[vis.entry_idx];
			return { m_tree[file_idx].plugin_idx, entry.rec_type, entry.record_id };
		}
	}

	return { -1, {}, {} };
}

void nav_tree_model_t::sort(int column, Qt::SortOrder order)
{
	if (column < 0 || column > 1)
		return;

	m_sort_column = column;
	m_sort_order = order;

	emit layoutAboutToBeChanged();
	sort_records();
	emit layoutChanged();
}

void nav_tree_model_t::sort_records()
{
	const auto & entries = m_scan.entries();
	const int column = m_sort_column;
	const bool ascending = (m_sort_order == Qt::AscendingOrder);

	for (auto & file_node : m_tree)
	{
		for (auto & group : file_node.groups)
		{
			std::sort(
			    group.records.begin(),
			    group.records.end(),
			    [&](const visible_record_t & a, const visible_record_t & b)
			{
				const auto & entry_a = entries[a.entry_idx];
				const auto & entry_b = entries[b.entry_idx];

				const std::string & name_a = (column == 0) ? entry_a.record_id : entry_a.display_name;
				const std::string & name_b = (column == 0) ? entry_b.record_id : entry_b.display_name;

				const int cmp = natural_compare(name_a, name_b);
				return ascending ? (cmp < 0) : (cmp > 0);
			});
		}
	}
}

QModelIndex nav_tree_model_t::index_for_root_level(int row, int column) const
{
	if (row < 0 || row >= static_cast<int>(m_tree.size()))
		return {};

	return createIndex(row, column, nullptr);
}

QModelIndex nav_tree_model_t::index_for_esm_group(void * ptr, int parent_row, int row, int column) const
{
	const auto * file_ptr = static_cast<const file_node_t *>(ptr);
	for (size_t file_idx = 0; file_idx < m_tree.size(); ++file_idx)
	{
		if (&m_tree[file_idx] != file_ptr)
			continue;

		if (parent_row < 0 || parent_row >= static_cast<int>(file_ptr->groups.size()))
			return {};

		const auto & group = file_ptr->groups[static_cast<size_t>(parent_row)];
		if (row < 0 || row >= static_cast<int>(group.records.size()))
			return {};

		return createIndex(row, column, const_cast<type_group_t *>(&group));
	}

	return {};
}

QVariant nav_tree_model_t::data_for_root_level(int row, int column, int role) const
{
	if (row < 0 || row >= static_cast<int>(m_tree.size()))
		return {};

	return data_for_file_node(row, column, role);
}

QVariant nav_tree_model_t::data_for_file_node(int row, int column, int role) const
{
	const auto & file_node = m_tree[static_cast<size_t>(row)];

	if (role == Qt::DisplayRole && column == 0)
		return file_node_display_text(file_node);

	if (role == Qt::BackgroundRole || role == Qt::ForegroundRole || role == Qt::FontRole)
		return file_node_appearance(file_node, role);

	return {};
}

QVariant nav_tree_model_t::file_node_display_text(const file_node_t & file_node) const
{
	char display_buffer[64];
	std::snprintf(
	    display_buffer,
	    sizeof(display_buffer),
	    "[%03d] %s",
	    file_node.plugin_idx,
	    m_scan.plugin_filename(file_node.plugin_idx).c_str());

	const auto & filename = m_scan.plugin_filename(file_node.plugin_idx);

	if (m_filter.excluded_plugins() && m_filter.excluded_plugins()->count(filename))
		return QString::fromUtf8("\xF0\x9F\x94\x92 ") + QString::fromUtf8(display_buffer);

	if (m_filter.patch_plugins() && m_filter.patch_plugins()->count(filename))
		return QString::fromUtf8("\xF0\x9F\x9B\xA1 ") + QString::fromUtf8(display_buffer);

	if (m_scan.is_merge_plugin(file_node.plugin_idx))
		return QString::fromUtf8("\xE2\x9A\x99 ") + QString::fromUtf8(display_buffer);

	if (m_editable_columns && m_editable_columns->is_plugin_editable(file_node.plugin_idx))
		return QString::fromUtf8("\xE2\x9C\x8D ") + QString::fromUtf8(display_buffer);

	const auto & full_path = m_scan.plugin_path(file_node.plugin_idx);
	const bool is_overridden =
	    full_path.find("/overwrite/") != std::string::npos || full_path.find("\\overwrite\\") != std::string::npos;

	const bool is_master = filename.size() > 4 && (filename.compare(filename.size() - 4, 4, ".esm") == 0 ||
	                                               filename.compare(filename.size() - 4, 4, ".ESM") == 0);
	if (is_master)
		return QString::fromUtf8("\xF0\x9F\x93\x9C ") + QString::fromUtf8(display_buffer);

	if (is_overridden)
		return QString::fromUtf8("\xE2\x9A\xA1 ") + QString::fromUtf8(display_buffer);

	return QString::fromUtf8("\xF0\x9F\x93\x84 ") + QString::fromUtf8(display_buffer);
}

QVariant nav_tree_model_t::file_node_appearance(const file_node_t & file_node, int role) const
{
	const auto & entries = m_scan.entries();
	conflict_all_t worst_all = conflict_all_t::only_one;
	conflict_this_t worst_this = conflict_this_t::unknown;

	for (const auto & group : file_node.groups)
	{
		for (const auto & rec : group.records)
		{
			const auto & entry = entries[rec.entry_idx];
			const auto this_color = record_foreground_for_plugin(entry, file_node.plugin_idx);

			if (conflict_this_priority(this_color) > conflict_this_priority(worst_this))
				worst_this = this_color;

			if (entry.conflict_all > worst_all)
				worst_all = entry.conflict_all;
		}
	}

	if (role == Qt::BackgroundRole)
	{
		if (worst_all < conflict_all_t::no_conflict)
			return {};

		return QBrush(theme_system_t::instance().conflict_all_background(worst_all));
	}

	if (role == Qt::ForegroundRole)
	{
		if (worst_this == conflict_this_t::unknown)
			return {};

		return QBrush(theme_system_t::instance().conflict_this_foreground(worst_this));
	}

	return {};
}

QVariant nav_tree_model_t::data_for_esm_nodes(void * ptr, int row, int column, int role) const
{
	(void)m_scan.entries();

	for (size_t file_idx = 0; file_idx < m_tree.size(); ++file_idx)
	{
		if (ptr == &m_tree[file_idx])
			return data_for_type_group(file_idx, row, column, role);

		for (size_t group_idx = 0; group_idx < m_tree[file_idx].groups.size(); ++group_idx)
		{
			if (ptr != &m_tree[file_idx].groups[group_idx])
				continue;

			return data_for_record(file_idx, group_idx, row, column, role);
		}
	}

	return {};
}

QVariant nav_tree_model_t::data_for_type_group(size_t file_idx, int row, int column, int role) const
{
	if (row < 0 || row >= static_cast<int>(m_tree[file_idx].groups.size()))
		return {};

	const auto & entries = m_scan.entries();
	const auto & group = m_tree[file_idx].groups[static_cast<size_t>(row)];

	if (role == Qt::DisplayRole && column == 0)
	{
		if (group.type == "TES3")
			return tr("File Header");

		const char * display_name = type_to_display_name(group.type);
		if (display_name)
			return QString("%1 [%2]").arg(display_name).arg(group.records.size());

		return QString("%1 (%2)").arg(QString::fromStdString(group.type)).arg(group.records.size());
	}

	if (role != Qt::BackgroundRole && role != Qt::ForegroundRole && role != Qt::FontRole)
		return {};

	conflict_all_t worst_all = conflict_all_t::only_one;
	conflict_this_t worst_this = conflict_this_t::unknown;

	for (const auto & rec : group.records)
	{
		const auto & entry = entries[rec.entry_idx];
		const auto this_color = record_foreground_for_plugin(entry, m_tree[file_idx].plugin_idx);

		if (conflict_this_priority(this_color) > conflict_this_priority(worst_this))
			worst_this = this_color;

		if (entry.conflict_all > worst_all)
			worst_all = entry.conflict_all;
	}

	if (role == Qt::BackgroundRole)
	{
		if (worst_all < conflict_all_t::no_conflict)
			return {};

		return QBrush(theme_system_t::instance().conflict_all_background(worst_all));
	}

	if (role == Qt::ForegroundRole)
	{
		if (worst_this == conflict_this_t::unknown)
			return {};

		return QBrush(theme_system_t::instance().conflict_this_foreground(worst_this));
	}

	if (role == Qt::FontRole && m_show_deleted_strikeout)
	{
		for (const auto & rec : group.records)
		{
			if (entries[rec.entry_idx].has_dele)
			{
				QFont font;
				font.setStrikeOut(true);
				return font;
			}
		}
	}

	return {};
}

QVariant nav_tree_model_t::data_for_record(size_t file_idx, size_t group_idx, int row, int column, int role) const
{
	const auto & entries = m_scan.entries();
	const auto & group = m_tree[file_idx].groups[group_idx];

	if (row < 0 || row >= static_cast<int>(group.records.size()))
		return {};

	const auto & vis = group.records[static_cast<size_t>(row)];
	const auto & entry = entries[vis.entry_idx];
	const auto record_color = record_foreground_for_plugin(entry, m_tree[file_idx].plugin_idx);

	if (role == Qt::DisplayRole)
	{
		if (column == 0)
		{
			auto display_id = QString::fromUtf8(decode_to_utf8(entry.record_id, m_display_codepage));
			display_id.replace('|', " #");
			return display_id;
		}

		if (column == 1)
		{
			if (!entry.display_name.empty())
				return QString::fromUtf8(decode_to_utf8(entry.display_name, m_display_codepage));

			if (entry.rec_type == "INFO")
				return {};

			return QString::fromUtf8(decode_to_utf8(entry.dial_name, m_display_codepage));
		}
	}

	if (role == Qt::BackgroundRole)
	{
		if (entry.conflict_all < conflict_all_t::no_conflict)
			return {};

		if (m_filter.hide_duplicates() && unique_plugin_count(entry) <= 1)
			return {};

		return QBrush(theme_system_t::instance().conflict_all_background(entry.conflict_all));
	}

	if (role == Qt::ForegroundRole)
	{
		if (record_color == conflict_this_t::unknown)
			return {};

		return QBrush(theme_system_t::instance().conflict_this_foreground(record_color));
	}

	if (role == Qt::FontRole && m_show_deleted_strikeout && entry.has_dele)
	{
		QFont font;
		font.setStrikeOut(true);
		return font;
	}

	return {};
}
